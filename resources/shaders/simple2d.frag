#version 330 core

in vec2 gTextureCoords;

out vec4 gFragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;
uniform vec4 uTint;

void main() {
    gFragColor = uUseTexture * uTint * texture( uTexture, gTextureCoords ) +
                 ( 1 - uUseTexture ) * uTint;
}
