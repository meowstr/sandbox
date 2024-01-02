#version 330 core

in vec2 gTextureCoords;

out vec4 gFragColor;

uniform sampler2D uTexture;
uniform float uTime;

highp float random1(float seed) {
   return fract(sin(seed * 213987.2) * 43758.5453);
}

vec4 textureColor(float y) {
    return texture(uTexture, vec2(gTextureCoords.x, y));
}

vec4 offsetTextureColor(float y) {
    float blankingLineY = fract(uTime * 2);
    float blankingOffset = -0.05 * pow(5, -pow((blankingLineY - gTextureCoords.y) * 20.0, 2));

    float uniformOffset = -random1(gTextureCoords.y + fract(uTime)) * 0.005;

    float offset = blankingOffset + uniformOffset;

    return texture(uTexture, vec2(gTextureCoords.x + offset, y));
}


float randomBarIntensity() {
    float seed = round(fract(uTime * 0.51232) * 40);
    float randomLineY = random1(seed);
    float diff = abs(gTextureCoords.y - randomLineY);

    if (diff < (0.02 * random1(seed * 0.22141)) + 0.01) {
        return random1(seed * 0.41232);
    } else {
        return 0.0;
    }
}

float randomTearY() {
    return gTextureCoords.y - 0.3 * randomBarIntensity();
}

void main()
{
    float y = randomTearY();
    vec4 texColor = mix(offsetTextureColor(y), textureColor(y), 0.1);
    gFragColor = texColor;

}
