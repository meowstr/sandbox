#version 330 core

in vec2 gTextureCoords;

out vec4 gFragColor;

uniform sampler2D uTexture;

void main()
{
    gFragColor = texture(uTexture, gTextureCoords);
}
