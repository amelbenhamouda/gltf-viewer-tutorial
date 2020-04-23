#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;
in mat3 TBN;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;
uniform vec3 uEmissiveFactor;

uniform vec4 uBaseColorFactor;

uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uNormalScale;
uniform float uActiveNormal;// sert de booléen pour l'activation de la normal map (1 pour activer 0 sinon)

uniform sampler2D uBaseColorTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uEmissiveTexture;

///cube light
struct PoncLigth {
     vec3 LightPosition;
     vec3 CubeIntensity;
     float CubeDist; //distance d'atténuation
     //uint uCubeNumber
};

struct SpotLigth {
     vec3 LightPosition;
     vec3 LightIntensity;
     vec3 LightDirection;
     float CutOff;
     float OuterCutOff;
     float DistAttenuation; //distance d'atténuation
};


#define NB_PONC_LIGHTS 4
uniform PoncLigth pointLights[NB_PONC_LIGHTS];
uniform SpotLigth spotligth;
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


vec3 getNormal(){
    vec3 N = vec3(0,0,0);
    if (uActiveNormal>0.5){
        vec4 baseNormalFromTexture = texture(uNormalTexture, vTexCoords);
        vec3 normalTexture = uNormalScale*baseNormalFromTexture.rgb;
        N = normalize(normalTexture * 2.0 - 1.0);
        N = N * vec3(1,-1,1); //
    } else{
        N = normalize(vViewSpaceNormal);
    }
    return N;
}

vec3 directional(){


    vec3 N = getNormal();

    vec3 L = vec3(0,0,0);
    vec3 V = vec3(0,0,0);

    if (uActiveNormal>0.5){
        L = TBN*uLightDirection;
        V = TBN*normalize(-vViewSpacePosition);
    } else{
       	L = uLightDirection;
        V = normalize(-vViewSpacePosition);
    }


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
    vec3 N = getNormal();

    vec3 L = vec3(0,0,0);
    vec3 V = vec3(0,0,0);

    if (uActiveNormal>0.5){
        L = TBN*normalize(ligth.LightPosition-vViewSpacePosition);
        V = TBN*normalize(-vViewSpacePosition);
    } else{
        L = normalize(ligth.LightPosition-vViewSpacePosition);
        V = normalize(-vViewSpacePosition);
    }


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
	float dist = distance(ligth.LightPosition,vViewSpacePosition);
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


vec3 spotligthCalc(SpotLigth spotligth){

    vec3 N = getNormal();

    vec3 L = vec3(0,0,0);
    vec3 V = vec3(0,0,0);

    if (uActiveNormal>0.5){
        L = normalize(spotligth.LightPosition - vViewSpacePosition);
        V = TBN*normalize(-vViewSpacePosition);

    } else{
        L = normalize(spotligth.LightPosition - vViewSpacePosition);
        V = normalize(-vViewSpacePosition);
    }

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
	float dist = length(spotligth.LightPosition-vViewSpacePosition);
	float Kc = 1.f ,Kl,Kd;
	if(spotligth.DistAttenuation < 7 &&  spotligth.DistAttenuation>=0){       Kl = 0.7f;  Kd = 1.8f;}
    else if(spotligth.DistAttenuation < 13 && spotligth.DistAttenuation>=7){  Kl = 0.35f; Kd = 0.44f;}
    else if(spotligth.DistAttenuation < 20 && spotligth.DistAttenuation>=13){ Kl = 0.22f;   Kd = 0.20f;}
    else if(spotligth.DistAttenuation < 32 && spotligth.DistAttenuation>=20){ Kl = 0.14f; Kd = 0.07f;}
    else if(spotligth.DistAttenuation < 50 && spotligth.DistAttenuation>=32){ Kl = 0.09f; Kd = 0.032f;}
    else if(spotligth.DistAttenuation < 65 && spotligth.DistAttenuation>=50){ Kl = 0.07f; Kd = 0.017f;}
    else{                           Kl = 0.045f;Kd = 0.0075f;}
    float attenuation = 1.f/(Kc + Kl*dist + Kd*dist*dist);

    float theta = dot(L, normalize(-spotligth.LightDirection));
    float epsilon = spotligth.CutOff - spotligth.OuterCutOff;
    float intensity = clamp((theta - spotligth.OuterCutOff) / epsilon, 0.0, 1.0);

    vec3 spotlum = max(LINEARtoSRGB((f_diffuse + f_specular) * spotligth.LightIntensity * NdotL * attenuation),vec3(0));
	return spotlum*intensity;
}

void main() {

    vec4 emissiveTexture = SRGBtoLINEAR(texture(uEmissiveTexture, vTexCoords));
	vec3 emissive = uEmissiveFactor * emissiveTexture.rgb;
	vec3 result = directional() ;
	for(int i=0; i<NB_PONC_LIGHTS; i++){
        result += ponctual(pointLights[i]);
	}
    //float theta = dot(lightDir, normalize(-light.direction));
    //float epsilon = light.cutOff - light.outerCutOff;
    //float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
   // vec3 lightDir = normalize(spotligth.LightPosition - vViewSpacePosition);
   // float theta = dot(lightDir, normalize(spotligth.LightDirection ));
   // if (theta > cos(radians(spotligth.CuteOff))) {
   //     result += spotligthCalc(spotligth,lightDir);
    //}
	// fColor = LINEARtoSRGB(diffuse * uLightIntensity);
	result += spotligthCalc(spotligth);
	fColor = clamp(result + emissive,0,1);
}
