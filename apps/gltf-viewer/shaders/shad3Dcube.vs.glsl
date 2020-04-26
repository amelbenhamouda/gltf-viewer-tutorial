#version 330 core

// Attributs de sommet
layout(location = 0) in vec3 aVertexPosition; // Position du sommet
layout(location = 1) in vec3 aVertexNormal; // Normale du sommet
layout(location = 2) in vec2 aVertexTexCoords; // Coordonnées de texture du sommet

uniform float uSize_cube;
uniform mat4 uVMatrix;
uniform vec3 uPosCube;
uniform mat4 uPMatrix;

mat4 translate(vec3 txyz) {
    return mat4(vec4(1, 0, 0, 0), vec4(0, 1, 0, 0), vec4(0, 0, 1, 0), vec4(txyz[0], txyz[1], txyz[2], 1));
}

mat4 scale(float sx, float sy, float sz) {
    return mat4(vec4(sx, 0, 0, 0), vec4(0, sy, 0, 0), vec4(0, 0, sz, 0), vec4(0, 0, 0, 1));
}

// Sorties du shader
out vec3 vPosition_vs; // Position du sommet transformé dans l'espace View
out vec3 vNormal_vs; // Normale du sommet transformé dans l'espace View
out vec2 vTexCoords; // Coordonnées de texture du sommet

void main() {
    // Passage en coordonnées homogènes
    vec4 vertexPosition = vec4(aVertexPosition, 1); //1 car il s'agit des points
    vec4 vertexNormal = vec4(aVertexNormal, 0);     // 0 car il s'agit d'une normal

    vTexCoords = aVertexTexCoords;
    mat4 MVMatrix = uVMatrix * translate(uPosCube);

    mat4 NormalMatrix = transpose(inverse(MVMatrix));
    float scale_sun = uSize_cube;
    mat4 MVPMatrix = uPMatrix * MVMatrix * scale(scale_sun, scale_sun, scale_sun);
    vPosition_vs = vec3(MVMatrix * vertexPosition);  // le vecteur toujours à droite de la matrice
    vNormal_vs = vec3(NormalMatrix * vertexNormal);

    // Calcul de la position projetée
    gl_Position = MVPMatrix * vertexPosition; // MVP model view projection
}
