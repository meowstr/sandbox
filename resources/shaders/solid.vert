#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTextureCoords;

out vec4 gColor;
out vec2 gTextureCoords;

uniform mat4 uProj; 

void main()
{
    gColor = aColor;
    gTextureCoords = aTextureCoords;

    gl_Position = uProj * vec4(aPos, 1.0);
}
