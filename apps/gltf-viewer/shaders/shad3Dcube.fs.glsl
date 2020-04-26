#version 330 core

in vec2 vTexCoords;
out vec3 fFragColor;
uniform sampler2D uTexture;

//// matériaux de l'objet
uniform vec3 uColor;

void main() {
    fFragColor = uColor;
}

