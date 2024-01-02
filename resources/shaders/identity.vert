#version 330 core

layout( location = 0 ) in vec2 aPos;

out vec2 gTextureCoords;

void main() {
    float tx = aPos.x + 0.5f;
    float ty = aPos.y + 0.5f;

    gTextureCoords = vec2( tx, ty );
    gl_Position = vec4( aPos.x * 2.0f, aPos.y * 2.0f, 0.0, 1.0 );
}
