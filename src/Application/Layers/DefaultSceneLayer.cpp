#include "DefaultSceneLayer.h"
// The Scene currently uses the lighting that came with the framework to show it off, but lighting I tried to build can be found in frag_environment_reflective.glsl
// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool Sepia = true;
	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		

		// The Scene currently uses the lighting that came with the framework to show it off, but lighting I tried to build can be found in frag_environment_reflective.glsl
		// ^^^
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		// Load in the meshes
		MeshResource::Sptr duckyMesh = ResourceManager::CreateAsset<MeshResource>("ducky.obj");
		MeshResource::Sptr bathroomMesh = ResourceManager::CreateAsset<MeshResource>("megaBathroom.obj");
		MeshResource::Sptr soapMesh = ResourceManager::CreateAsset<MeshResource>("soap.obj");
		MeshResource::Sptr toiletMesh = ResourceManager::CreateAsset<MeshResource>("toilet.obj");

		// Load in some textures
		Texture2D::Sptr    duckyTexture  = ResourceManager::CreateAsset<Texture2D>("textures/Ducky.png");
		Texture2D::Sptr    bathroomTexture     = ResourceManager::CreateAsset<Texture2D>("textures/BathroomTexture.png");
		Texture2D::Sptr    soapTexture    = ResourceManager::CreateAsset<Texture2D>("textures/soap.png");
		Texture2D::Sptr    toiletTexture      = ResourceManager::CreateAsset<Texture2D>("textures/Toilet.png");

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>(); 

		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lutSepia = ResourceManager::CreateAsset<Texture3D>("luts/Sepia.cube");  

		Texture3D::Sptr lutWarm = ResourceManager::CreateAsset<Texture3D>("luts/Warm.cube");

		Texture3D::Sptr lutCool = ResourceManager::CreateAsset<Texture3D>("luts/Cool.cube");

		// Configure the color correction LUT
			scene->SetColorLUT(lutSepia);
			scene->SetColorLUTWarm(lutWarm);
			scene->SetColorLUTCool(lutSepia);
		


		// Create our materials
		
		Material::Sptr toiletMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			toiletMaterial->Name = "Toilet";
			toiletMaterial->Set("u_Material.Diffuse", toiletTexture);
			toiletMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr soapMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			soapMaterial->Name = "Soap";
			soapMaterial->Set("u_Material.Diffuse", soapTexture);
			soapMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr bathroomMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			bathroomMaterial->Name = "Bathroom";
			bathroomMaterial->Set("u_Material.Diffuse", bathroomTexture);
			bathroomMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr duckyMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			duckyMaterial->Name = "Ducky";
			duckyMaterial->Set("u_Material.Diffuse", duckyTexture);
			duckyMaterial->Set("u_Material.Shininess", 0.1f);
		}


		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(-1.0f, 1.0f, 5.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 10.0f;

		scene->Lights[1].Position = glm::vec3(-2.0f, 1.0f, 10.0f);
		scene->Lights[1].Color = glm::vec3(1.0f, 1.0f, 1.0f);

		scene->Lights[2].Position = glm::vec3(-1.0f, 1.0f, 5.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 1.0f, 1.0f);

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ 7.52, -2.12, 8.54 });
			camera->SetRotation({ 58.79, 2, 86.69});

			camera->Add<SimpleCameraControl>();

		}

		// Set up all our sample objects
		// 
		GameObject::Sptr Toilet = scene->CreateGameObject("Toilet");
		{
			Toilet->SetPostion(glm::vec3(-2.23f, -0.4f, 0.96f));

			RenderComponent::Sptr renderer = Toilet->Add<RenderComponent>();
			renderer->SetMesh(toiletMesh);
			renderer->SetMaterial(toiletMaterial);
		}

		GameObject::Sptr Soap = scene->CreateGameObject("Soap");
		{
			Soap->SetPostion(glm::vec3(-4.38f, -3.27f, 3.74f));
			Soap->SetRotation(glm::vec3(0.f, 0.08f, 161.f));
			Soap->SetScale(glm::vec3(0.55f, 0.51f, 0.47f));

			RenderComponent::Sptr renderer = Soap->Add<RenderComponent>();
			renderer->SetMesh(soapMesh);
			renderer->SetMaterial(soapMaterial);
		}

		GameObject::Sptr Bathroom = scene->CreateGameObject("Bathroom");
		{
			RenderComponent::Sptr renderer = Bathroom->Add<RenderComponent>();
			renderer->SetMesh(bathroomMesh);
			renderer->SetMaterial(bathroomMaterial);
		}

		GameObject::Sptr Ducky = scene->CreateGameObject("Ducky");
		{
			Ducky->SetPostion(glm::vec3(-1.11f, 4.02f, 1.86f));
			Ducky->SetRotation(glm::vec3(6.f, 0.08f, -60.0f));
			Ducky->SetScale(glm::vec3(0.6f, 0.6f, 0.6f));

			RenderComponent::Sptr renderer = Ducky->Add<RenderComponent>();
			renderer->SetMesh(duckyMesh);
			renderer->SetMaterial(duckyMaterial);

			RotatingBehaviour::Sptr spin = Ducky->Add<RotatingBehaviour>();

			spin->RotationSpeed = glm::vec3(0.f, 0.f, 20.f);
		}
		

		GameObject::Sptr demoBase = scene->CreateGameObject("Demo Parent");


		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
