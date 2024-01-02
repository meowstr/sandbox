#include "Graphics.h"

#include "Test.h"

namespace engine::graphics {

// make sure the struct packing isn't weird
static_assert( sizeof( Vertex ) == sizeof( GLfloat ) * 8 );

void initBuffer( Buffer & buffer ) {
    // allocate
    buffer.vao.init();
    buffer.vbo.init();

    // setup bindings
    glBindVertexArray( buffer.vao );
    glBindBuffer( GL_ARRAY_BUFFER, buffer.vbo );

    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );
    glEnableVertexAttribArray( 2 );

    // position
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof( GLfloat ),
                           (void *) ( 0 ) );
    // normal
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof( GLfloat ),
                           (void *) ( 3 * sizeof( GLfloat ) ) );
    // uv
    glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof( GLfloat ),
                           (void *) ( 6 * sizeof( GLfloat ) ) );

    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    buffer.vertexCount = 0;
}

void writeBuffer( Buffer & buffer, const std::vector< Vertex > & vs ) {
    glBindBuffer( GL_ARRAY_BUFFER, buffer.vbo );
    glBufferData( GL_ARRAY_BUFFER, vs.size() * sizeof( Vertex ), vs.data(),
                  GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    buffer.vertexCount = vs.size();
}

void draw( const Buffer & buffer ) {
    draw( buffer.vao, buffer.vertexCount );
}

static_assert( sizeof( glm::vec2 ) == sizeof( GLfloat ) * 2 );

void initBuffer( Buffer2D & buffer ) {
    // allocate
    buffer.vao.init();
    buffer.posVbo.init();
    buffer.uvVbo.init();

    // setup bindings
    glBindVertexArray( buffer.vao );

    // position
    glBindBuffer( GL_ARRAY_BUFFER, buffer.posVbo );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof( GLfloat ),
                           (void *) ( 0 ) );

    // uvs
    glBindBuffer( GL_ARRAY_BUFFER, buffer.uvVbo );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof( GLfloat ),
                           (void *) ( 0 ) );

    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    buffer.vertexCount = 0;
}

void writeBuffer( Buffer2D & buffer, const std::vector< glm::vec2 > & positions,
                  const std::vector< glm::vec2 > & uvs ) {

    LOGGER_ASSERT( positions.size() == uvs.size() );

    glBindBuffer( GL_ARRAY_BUFFER, buffer.posVbo );
    glBufferData( GL_ARRAY_BUFFER, positions.size() * sizeof( glm::vec2 ),
                  positions.data(), GL_STREAM_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, buffer.uvVbo );
    glBufferData( GL_ARRAY_BUFFER, uvs.size() * sizeof( glm::vec2 ), uvs.data(),
                  GL_STREAM_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    buffer.vertexCount = positions.size();
}

void draw( const Buffer2D & buffer ) {
    draw( buffer.vao, buffer.vertexCount );
}

void draw( const gl::VertexArray & vao, size_t vertexCount ) {
    glBindVertexArray( vao );
    glDrawArrays( GL_TRIANGLES, 0, vertexCount );
}

void initTextBuffer( TextBuffer & buffer ) {
    initBuffer( buffer.buffer );
}

void writeTextBuffer( TextBuffer & buffer, const BitmapFont & font,
                      const std::string & text, glm::vec2 pos, float scale,
                      TextAlignment alignment ) {
    buffer.glyphs.clear();
    buffer.size = engine::glyphs( buffer.glyphs, font, text );

    // compute transform
    int dx;
    int dy;

    buffer.size.width *= scale;
    buffer.size.height *= scale;

    switch ( alignment ) {
    case TextAlignment::TopLeft:
        dx = (int) pos.x;
        dy = (int) pos.y;
        break;
    case TextAlignment::Center:
        dx = (int) ( pos.x - buffer.size.width * 0.5f );
        dy = (int) ( pos.y - buffer.size.height * 0.5f );
        break;
    case TextAlignment::CenterX:
        dx = (int) ( pos.x - buffer.size.width * 0.5f );
        break;
    case TextAlignment::CenterY:
        dy = (int) ( pos.y - buffer.size.height * 0.5f );
        break;
    }

    buffer.positions.clear();
    buffer.uvs.clear();

    for ( const engine::Glyph & g : buffer.glyphs ) {
        Vertex2D v1; // bottom-left
        Vertex2D v2; // top-left
        Vertex2D v3; // top-right
        Vertex2D v4; // bottom-left
        Vertex2D v5; // top-right
        Vertex2D v6; // bottom-right

        // bottom-left
        v1.pos.x = g.x * scale + dx;
        v1.pos.y = ( g.y + g.h ) * scale + dy;
        v1.uv.x = g.u1;
        v1.uv.y = g.v1;

        // top-left
        v2.pos.x = g.x * scale + dx;
        v2.pos.y = g.y * scale + dy;
        v2.uv.x = g.u1;
        v2.uv.y = g.v2;

        // top-right
        v3.pos.x = ( g.x + g.w ) * scale + dx;
        v3.pos.y = g.y * scale + dy;
        v3.uv.x = g.u2;
        v3.uv.y = g.v2;

        v4 = v1;

        v5 = v3;

        // bottom-right
        v6.pos.x = ( g.x + g.w ) * scale + dx;
        v6.pos.y = ( g.y + g.h ) * scale + dy;
        v6.uv.x = g.u2;
        v6.uv.y = g.v1;

        buffer.positions.push_back( v1.pos );
        buffer.positions.push_back( v2.pos );
        buffer.positions.push_back( v3.pos );
        buffer.positions.push_back( v4.pos );
        buffer.positions.push_back( v5.pos );
        buffer.positions.push_back( v6.pos );

        buffer.uvs.push_back( v1.uv );
        buffer.uvs.push_back( v2.uv );
        buffer.uvs.push_back( v3.uv );
        buffer.uvs.push_back( v4.uv );
        buffer.uvs.push_back( v5.uv );
        buffer.uvs.push_back( v6.uv );
    }

    writeBuffer( buffer.buffer, buffer.positions, buffer.uvs );
}

void draw( const TextBuffer & buffer ) {
    draw( buffer.buffer );
}

} // namespace engine::graphics
