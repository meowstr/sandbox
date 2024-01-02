#pragma once

#include <glad/glad.h>

#include "Logging.h"

namespace gl {

class GlName {
  public:
    GlName() = default;
    ~GlName() {
        // DEBUG_LOG() << "Deleting gl name." << std::endl;
    }

    // remove copy semantics
    GlName( const GlName & ) = delete;
    GlName & operator=( const GlName & ) = delete;

    // add move semantics
    GlName( GlName && src ) : name( src.name ) {
        // invalidate
        src.name = 0;
    }

    GlName & operator=( GlName && src ) {
        if ( &src != this ) {
            name = src.name;
            // invalidate
            src.name = 0;
        }

        return *this;
    }

    unsigned int & get() {
        LOGGER_ASSERT( valid );
        return name;
    }

    const unsigned int & get() const {
        LOGGER_ASSERT( valid );
        return name;
    }

    unsigned int name;
    int valid = 0;
};

#define DEFINE_GL_RAII1( cName, createFunc, destroyFunc )                      \
    class cName {                                                              \
      public:                                                                  \
        operator unsigned int() const {                                        \
            return mInternal.get();                                            \
        }                                                                      \
        ~cName() {                                                             \
            if ( mInternal.valid ) {                                           \
                destroyFunc( 1, &( mInternal.name ) );                         \
            }                                                                  \
        }                                                                      \
        void init() {                                                          \
            unsigned int x;                                                    \
            createFunc( 1, &x );                                               \
            reset( x );                                                        \
        }                                                                      \
        void reset( unsigned int name ) {                                      \
            if ( mInternal.valid ) {                                           \
                destroyFunc( 1, &mInternal.name );                             \
            }                                                                  \
            mInternal.name = name;                                             \
            mInternal.valid = 1;                                               \
        }                                                                      \
                                                                               \
      private:                                                                 \
        GlName mInternal;                                                      \
    };

#define DEFINE_GL_RAII2( cName, destroyFunc )                                  \
    class cName {                                                              \
      public:                                                                  \
        operator unsigned int() const {                                        \
            return mInternal.get();                                            \
        }                                                                      \
        ~cName() {                                                             \
            if ( mInternal.valid ) {                                           \
                destroyFunc( mInternal.name );                                 \
            }                                                                  \
        }                                                                      \
        void reset( unsigned int name ) {                                      \
            if ( mInternal.valid ) {                                           \
                destroyFunc( mInternal.name );                                 \
            }                                                                  \
            mInternal.name = name;                                             \
            mInternal.valid = 1;                                               \
        }                                                                      \
                                                                               \
      private:                                                                 \
        GlName mInternal;                                                      \
    };

/// automated raii's
////////////////////////////////////////////////////////////////////////////////

DEFINE_GL_RAII1( Buffer, glGenBuffers, glDeleteBuffers );
DEFINE_GL_RAII1( VertexArray, glGenVertexArrays, glDeleteVertexArrays );
DEFINE_GL_RAII1( Framebuffer, glGenFramebuffers, glDeleteFramebuffers );
DEFINE_GL_RAII1( Texture, glGenTextures, glDeleteTextures );
DEFINE_GL_RAII1( RenderBuffer, glGenRenderbuffers, glDeleteRenderbuffers );
DEFINE_GL_RAII2( ShaderProgram, glDeleteProgram );
DEFINE_GL_RAII2( Shader, glDeleteShader );

} // namespace gl
