#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;

uniform vec4 uBaseColorFactor;

uniform float uMetallicFactor;
uniform float uRoughnessFactor;

uniform sampler2D uBaseColorTexture;
uniform sampler2D uMetallicRoughnessTexture;

out vec3 fColor;

// Constants
const float GAMMA = 2.2;
const float INV_GAMMA = 1. / GAMMA;
const float M_PI = 3.141592653589793;
const float M_1_PI = 1.0 / M_PI;

// We need some simple tone mapping functions
// Basic gamma = 2.2 implementation
// Stolen here: https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/tonemapping.glsl

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 LINEARtoSRGB(vec3 color) { 
	return pow(color, vec3(INV_GAMMA));
}

// sRGB to linear approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec4 SRGBtoLINEAR(vec4 srgbIn) {
	return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w);
}

void main() {
	vec3 N = normalize(vViewSpaceNormal);
	vec3 L = uLightDirection;
	vec3 V = normalize(-vViewSpacePosition);
	vec3 H = normalize(L + V);

	vec4 baseColorFromTexture = SRGBtoLINEAR(texture(uBaseColorTexture, vTexCoords));
	vec4 metallicRougnessFromTexture = texture(uMetallicRoughnessTexture, vTexCoords);
	vec4 baseColor = uBaseColorFactor * baseColorFromTexture;

	vec3 metallic = vec3(uMetallicFactor * metallicRougnessFromTexture.b);

  	float roughness = uRoughnessFactor * metallicRougnessFromTexture.g;

  	vec3 dielectricSpecular = vec3(0.04, 0.04, 0.04);
	vec3 black = vec3(0, 0, 0);

	vec3 c_diff = mix(baseColor.rgb * (1 - dielectricSpecular.r), black, metallic);
  	vec3 F_0 = mix(dielectricSpecular, baseColor.rgb, metallic);
  	float alpha = roughness * roughness;

  	float baseShlickFactor = 1 - clamp(dot(V, N), 0, 1);
	// You need to compute baseShlickFactor first
	float shlickFactor = baseShlickFactor * baseShlickFactor; // power 2
	shlickFactor *= shlickFactor; // power 4
	shlickFactor *= baseShlickFactor; // power 5

	vec3 F = F_0 + (vec3(1) - F_0) * shlickFactor;
	float NdotL = clamp(dot(N, L), 0, 1);
    float NdotV = clamp(dot(N, V), 0, 1);

    float sqrAlpha = alpha * alpha;
    float visDen = NdotL * sqrt(NdotV * NdotV * (1 - sqrAlpha) + sqrAlpha) + NdotV * sqrt(NdotL * NdotL * (1 - sqrAlpha) + sqrAlpha);
    float Vis = visDen > 0. ? 0.5 / visDen : 0.0;

    float NdotH = clamp(dot(N, H), 0, 1);
	float dDen = (NdotH * NdotH * (sqrAlpha - 1) + 1);
	float D = M_1_PI * sqrAlpha / (dDen * dDen);

	vec3 f_specular = F * Vis * D;
	
	// vec3 diffuse = baseColorFromTexture.rgb * M_1_PI * NdotL;
	// vec3 diffuse = baseColor.rgb * M_1_PI * NdotL;
	vec3 diffuse = c_diff * M_1_PI;

	vec3 f_diffuse = (1 - F) * diffuse;

	// fColor = LINEARtoSRGB(diffuse * uLightIntensity);
	fColor = LINEARtoSRGB((f_diffuse + f_specular) * uLightIntensity * NdotL);
}
