#version 330 core

in vec4 gColor;
in vec2 gTextureCoords;

out vec4 gFragColor;

uniform sampler2D uTexture;
uniform float uTintIntensity;

vec4 textureColor() {
    return texture(uTexture, gTextureCoords);
}

void main()
{
    vec4 baseColor = gColor * textureColor();
    gFragColor = mix(baseColor, gColor, uTintIntensity) ;
    //gFragColor = gColor;
}
