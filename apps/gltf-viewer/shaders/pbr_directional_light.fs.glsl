#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;


uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;
uniform vec3 uEmissiveFactor;

uniform vec4 uBaseColorFactor;

uniform float uMetallicFactor;
uniform float uRoughnessFactor;

uniform sampler2D uBaseColorTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uEmissiveTexture;

///cube light
struct PoncLigth {
     vec3 LightPosition;
     vec3 CubeIntensity;
     float CubeDist;
     //uint uCubeNumber
};
#define NB_PONC_LIGHTS 4
uniform PoncLigth pointLights[NB_PONC_LIGHTS];

/*
uniform vec3 uLightPosition;
uniform vec3 uCubeIntensity;
uniform vec3 uCubeColor;
uniform float uCubeDist;
uniform uint uCubeNumber*/
///
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


vec3 directional(){
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
    //float Vis = visDen > 0. ? 0.5 / visDen : 0.0;
    float Vis =  0.5 / visDen ;
    float NdotH = clamp(dot(N, H), 0, 1);
	float dDen = (NdotH * NdotH * (sqrAlpha - 1) + 1);
	float D = M_1_PI * sqrAlpha / (dDen * dDen);
	vec3 f_specular = F * Vis * D;
	vec3 diffuse = c_diff * M_1_PI;
	vec3 f_diffuse = (1 - F) * diffuse;
	return max(LINEARtoSRGB((f_diffuse + f_specular) * uLightIntensity * NdotL),vec3(0));
}
vec3 ponctual(PoncLigth ligth){
    vec3 N = normalize(vViewSpaceNormal);
	vec3 L = normalize(ligth.LightPosition-vViewSpacePosition);
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
    float Vis = visDen != 0. ? 0.5 / visDen : 0.0;

    float NdotH = clamp(dot(N, H), 0, 1);
	float dDen = (NdotH * NdotH * (sqrAlpha - 1) + 1);
	float D = M_1_PI * sqrAlpha / (dDen * dDen);
	vec3 f_specular = F * Vis * D;
	vec3 diffuse = c_diff * M_1_PI;
	vec3 f_diffuse = (1 - F) * diffuse;
	float dist = length(ligth.LightPosition-vViewSpacePosition);
	float Kc = 1.f ,Kl,Kd;
	if(ligth.CubeDist < 7 &&  ligth.CubeDist>=0){       Kl = 0.7f;  Kd = 1.8f;}
    else if(ligth.CubeDist < 13 && ligth.CubeDist>=7){  Kl = 0.35f; Kd = 0.44f;}
    else if(ligth.CubeDist < 20 && ligth.CubeDist>=13){ Kl = 0.22f;   Kd = 0.20f;}
    else if(ligth.CubeDist < 32 && ligth.CubeDist>=20){ Kl = 0.14f; Kd = 0.07f;}
    else if(ligth.CubeDist < 50 && ligth.CubeDist>=32){ Kl = 0.09f; Kd = 0.032f;}
    else if(ligth.CubeDist < 65 && ligth.CubeDist>=50){ Kl = 0.07f; Kd = 0.017f;}
    else{                           Kl = 0.045f;Kd = 0.0075f;}
    float attenuation = 1.f/(Kc + Kl*dist + Kd*dist*dist);
	return max(LINEARtoSRGB((f_diffuse + f_specular) * ligth.CubeIntensity * NdotL * attenuation),vec3(0));
}
void main() {

    vec4 emissiveTexture = SRGBtoLINEAR(texture(uEmissiveTexture, vTexCoords));
	vec3 emissive = uEmissiveFactor * emissiveTexture.rgb;
	vec3 result = directional() ;
	for(int i=0; i<NB_PONC_LIGHTS; i++){
        result += ponctual(pointLights[i]);
	}

	// fColor = LINEARtoSRGB(diffuse * uLightIntensity);
	fColor = clamp(result + emissive,0,1);
}
