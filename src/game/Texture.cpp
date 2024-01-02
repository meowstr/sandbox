#include "Texture.h"

#include "Logging.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/glad.h>

namespace engine {

void loadTexture( gl::Texture & texture, const std::filesystem::path & path ) {
    INFO_LOG() << "Loading texture " << path.generic_string() << "."
               << std::endl;

    // generate a texture
    glBindTexture( GL_TEXTURE_2D, texture );
    // set the texture wrapping/filtering options (on the currently bound
    // texture object)
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char * data =
        stbi_load( path.c_str(), &width, &height, &nrChannels, 0 );
    if ( data ) {
        // glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB,
        //               GL_UNSIGNED_BYTE, data );
        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
        if ( nrChannels == 3 ) {
            // DEBUG_LOG() << "RGB" << std::endl;
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                          GL_UNSIGNED_BYTE, data );
        } else {
            // DEBUG_LOG() << "RGBA" << std::endl;
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                          GL_UNSIGNED_BYTE, data );
        }
        // glGenerateMipmap( GL_TEXTURE_2D );
    } else {
        throw std::runtime_error( "Failed to load texture." );
    }
    stbi_image_free( data );
}

void genTexture( gl::Texture & texture, int width, int height ) {
    // generate a texture

    glBindTexture( GL_TEXTURE_2D, texture );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_2D, 0 );
}

void genDepthStencilTexture( gl::Texture & texture, int width, int height ) {
    glBindTexture( GL_TEXTURE_2D, texture );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
                  GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_2D, 0 );
}

// static std::map< std::string, std::unique_ptr< Texture > > cachedTextures;
//
// Texture * FindTexture( const std::string & name ) {
//     auto texture = cachedTextures.find( name );
//     if ( texture == cachedTextures.end() ) {
//         Texture * newTexture = LoadTexture( ResourcePath( name ) );
//         cachedTextures.emplace( name, newTexture );
//         return newTexture;
//     } else {
//         return texture->second.get();
//     }
// }

} // namespace engine
