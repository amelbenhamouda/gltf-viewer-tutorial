#version 330

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec4 aTangent;

out vec3 vViewSpacePosition;
out vec3 vViewSpaceNormal;
out vec2 vTexCoords;
out mat3 TBN;

uniform mat4 uModelViewProjMatrix;
uniform mat4 uModelViewMatrix;
uniform mat4 uNormalMatrix;

void main()
{
    vViewSpacePosition = vec3(uModelViewMatrix * vec4(aPosition, 1));
	vViewSpaceNormal = normalize(vec3(uNormalMatrix * vec4(aNormal, 0)));

    vec4 vViewSpaceTangent = normalize(uNormalMatrix * aTangent);
    vViewSpaceTangent =  normalize(vViewSpaceTangent-dot(vViewSpaceTangent,vec4(vViewSpaceNormal,0))*vec4(vViewSpaceNormal,0));
    vec3 B = cross(vViewSpaceNormal,vViewSpaceTangent.xyz)*vViewSpaceTangent.w;
    TBN = transpose(mat3(vViewSpaceTangent.xyz,B,vViewSpaceNormal)); // TBN matrix
	vTexCoords = aTexCoords;
    gl_Position =  uModelViewProjMatrix * vec4(aPosition, 1);
}
