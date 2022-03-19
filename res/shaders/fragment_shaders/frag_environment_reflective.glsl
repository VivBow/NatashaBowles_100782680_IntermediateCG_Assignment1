#version 440

#include "../fragments/fs_common_inputs.glsl"

// We output a single color to the color buffer
layout(location = 0) out vec4 frag_color;

////////////////////////////////////////////////////////////////
/////////////// Frame Level Uniforms ///////////////////////////
////////////////////////////////////////////////////////////////

#include "../fragments/frame_uniforms.glsl"

////////////////////////////////////////////////////////////////
/////////////// Instance Level Uniforms ////////////////////////
////////////////////////////////////////////////////////////////

// Represents a collection of attributes that would define a material
// For instance, you can think of this like material settings in 
// Unity
struct Material {
	sampler2D Diffuse;
	float     Shininess;
};
// Create a uniform for the material
uniform Material u_Material;

////////////////////////////////////////////////////////////////
///////////// Application Level Uniforms ///////////////////////
////////////////////////////////////////////////////////////////

#include "../fragments/multiple_point_lights.glsl"
#include "../fragments/color_correction.glsl"

const float LOG_MAX = 2.40823996531;

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
//	// Normalize our input normal
//	vec3 normal = normalize(inNormal);
//
//	vec3 toEye = normalize(u_CamPos.xyz - inWorldPos);
//	vec3 environmentDir = reflect(-toEye, normal);
//	vec3 reflected = SampleEnvironmentMap(environmentDir);
//
	// Will accumulate the contributions of all lights on this fragment
//	// This is defined in the fragment file "multiple_point_lights.glsl"
//	vec3 lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, u_Material.Shininess);

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(u_Material.Diffuse, inUV);
//
//	// combine for the final result
//	vec3 result = lightAccumulation  * inColor * textureColor.rgb;

	//Ambient stuff
	//Need colour and strength
	vec3 stronk = 0.1;

	vec3 ambientProduct = stronk * textureColor.rgb ;


	//Diffuse stuff
	// First need to get the normal

	vec3 normals = normalize(inNormal);

	//Now we find the dot product of that and the light direction

	vec3 impact = max( dot(normals, light.direction), 0.0);

	// finally multiply that by the light's colour to get the diffuse

	vec3 difuseFinal = inColor * impact;

	//Specular
	//I need to establish a specular intensity. The scene is a bathroom, which tends to have pretty bright reflections, so I will use 0.7

	float specStronk = 0.7;

	//Next is finding the reflection direction

	vec3 reflection = reflect (-light.direction, normals);

	//now to finish the calculations

	float specularComponent = pow(max (dot (u_CamPos.xyz, reflection), 0.0), 32);

	vec3 specularProduct = specStronk * specularComponent * inColor;


	frag_color = vec4(ColorCorrect(ambientProduct + difuseFinal + specularProduct), 1.0);


}