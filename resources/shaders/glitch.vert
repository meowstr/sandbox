#version 330 core

layout (location = 0) in vec2 aPos;

out vec2 gTextureCoords;

void main()
{
    float tx = (aPos.x + 1) * 0.5;
    float ty = (aPos.y + 1) * 0.5;

    gTextureCoords = vec2(tx, ty);
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
