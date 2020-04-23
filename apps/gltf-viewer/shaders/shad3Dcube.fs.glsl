#version 330 core

in vec2 vTexCoords;
out vec3 fFragColor;
uniform sampler2D uTexture;

//// matériaux de l'objet
uniform vec3 uColor;

void main() {
   /* vec3 text3D = vec3(texture(uTexture, vTexCoords)); // la texture/ les coordonnées
    if (text3D[0] != 0.0) {
        fFragColor = vec3(text3D) * uColor;
    }
    else {
        fFragColor = uColor;
    }*/

    fFragColor = uColor;
}

