#version 330 core

layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNormal;
layout( location = 2 ) in vec2 aTextureCoords;

out vec4 gColor;
out vec3 gNormal;
out vec3 gPos;
out vec2 gTextureCoords;

uniform mat4 uProj; // to native from world
uniform mat4 uView; // to world from input

uniform vec4 uTint;

void main() {
    // uView should not cause a perspective division
    vec4 normal = uView * vec4( aNormal, 0.0f );
    vec4 pos = uView * vec4( aPos, 1.0f );

    gColor = uTint;
    gPos = pos.xyz;
    gNormal = normal.xyz;
    gTextureCoords = aTextureCoords;

    gl_Position = uProj * pos;
}
