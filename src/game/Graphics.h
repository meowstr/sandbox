#pragma once

#include "Font.h"
#include "GlRaii.h"
#include "Vertex.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <vector>

namespace engine::graphics {

//#pragma pack( push, 1 )
// struct Vertex {
//    glm::vec3 position;
//    glm::vec3 normal;
//    glm::vec2 uv;
//};
//#pragma pack( pop )

struct Vertex2D {
    glm::vec2 pos;
    glm::vec2 uv;
};

struct Buffer {
    gl::VertexArray vao;
    gl::Buffer vbo;
    size_t vertexCount;
};

struct Buffer2D {
    gl::VertexArray vao;
    gl::Buffer posVbo;
    gl::Buffer uvVbo;
    size_t vertexCount;
};

struct TextBuffer {
    std::vector< Glyph > glyphs;
    std::vector< glm::vec2 > positions;
    std::vector< glm::vec2 > uvs;
    TextSize size;
    Buffer2D buffer;
};

enum TextAlignment {
    TopLeft,
    Center,
    CenterX,
    CenterY,
};

void initTextBuffer( TextBuffer & buffer );
void writeTextBuffer( TextBuffer & buffer, const BitmapFont & font,
                      const std::string & string, glm::vec2 pos, float scale,
                      TextAlignment alignment = TextAlignment::TopLeft );
void draw( const TextBuffer & buffer );

void initBuffer( Buffer2D & buffer );
void writeBuffer( Buffer2D & buffer, const std::vector< glm::vec2 > & vs,
                  const std::vector< glm::vec2 > & uvs );
void draw( const Buffer2D & buffer );

void initBuffer( Buffer & buffer );
void writeBuffer( Buffer & buffer, const std::vector< Vertex > & vs );
void draw( const Buffer & buffer );

void draw( const gl::VertexArray & vao, size_t vertexCount );

} // namespace engine::graphics
