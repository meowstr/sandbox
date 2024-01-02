#version 330 core

layout( location = 0 ) in vec2 aPos;
layout( location = 1 ) in vec2 aUv;

out vec2 gTextureCoords;

uniform mat4 uMat;

void main() {
    //float tx = aPos.x + 0.5;
    //float ty = aPos.y + 0.5;

    gTextureCoords = aUv;
    gl_Position = uMat * vec4( aPos.x, aPos.y, 0.0, 1.0 );
}
