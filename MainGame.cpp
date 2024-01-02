#include "Game.hpp"

#include "Font.hpp"
#include "GUI.hpp"
#include "Globals.hpp"
#include "Input.hpp"
#include "Light.hpp"
#include "Scheme.hpp"
#include "Shader.hpp"
#include "Timer.hpp"

// Scenes
#include "Liminal.hpp"
#include "Tycoon.hpp"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <entt/signal/sigh.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

struct ShadowShader {};

class Game::Private {
  public:
    Private( int width, int height );

    void Init();

    void InitScm();
    void ScmRun( const std::string & expr );

    void SayHi();
    void SayShit();

    void Tick( float timestep );
    void Render( float timestep );
    void Resize( int width, int height );

    void CenterCamera( glm::vec2 center );
    void ViewMouseDown( CursorInfo info );

    std::unique_ptr< MainGui > & Gui() {
        return gui_;
    }

  private:
    // width of window in pixels
    int windowWidth_;
    // height of window in pixels
    int windowHeight_;

    // global shader program
    std::unique_ptr< Shader > defaultShader_;

    // handles rendering to the shadow texture
    std::unique_ptr< ShadowMapper > shadowMapper_;

    // the whole gui
    std::unique_ptr< MainGui > gui_;

    // projection matrix
    glm::mat4 projMatrix_;
    // view matrix
    glm::mat4 viewMatrix_;
    // combiend matrix (proj * view)
    glm::mat4 combinedMatrix_;

    // combined matrix for GUI
    glm::mat4 guiCombinedMatrix_;

    // some cool fonts
    BitmapFont * font_;
    BitmapFont * codeFont_;

    // the main rendering batch
    std::unique_ptr< Batch > batch_;

    // the scheme engine
    Scm::Engine scheme_;

  public:
    // the tycoon engine
    std::unique_ptr< Tycoon > tycoon_;
    std::unique_ptr< Liminal > liminal_;
};

static Scm::Object Scream( Scm::Engine & engine, Scm::Object args ) {
    LOG( Logger::Debug, "AHHHHHHH" );

    return engine.Primitives.Void;
}

static void Scream1() {
    LOG( Logger::Debug, "AHHHHHHH" );
}

static void Scream2() {
    LOG( Logger::Debug, "EEEEEHHH" );
}

static CursorInfo cursor;

// TODO: do more of this
entt::sigh< void( void ) > OnShitted;
entt::sink Shitted{ OnShitted };

std::vector< ManualTimer > renderTimers;

const float CAMERA_VIEW_DISTANCE = 20;

static Scm::Object Interact( Scm::Engine & engine, Tycoon & tycoon,
                             Scm::Object args ) {
    auto x = Scm::Get< Scm::FixNum >( engine.Car( args ) );
    auto y = Scm::Get< Scm::FixNum >( engine.Car( engine.Cdr( args ) ) );

    tycoon.Interact( x.value, y.value );

    return engine.Primitives.Void;
}

Game::Private::Private( int width, int height ) {
    // do some init
    Init();

    // resize for the first time
    Resize( width, height );
}

bool debugInteract = false;

void Game::Private::ViewMouseDown( CursorInfo info ) {
    // determine depth of the pixel
    float z = 1;
    glReadPixels( info.x, windowHeight_ - info.y, 1, 1, GL_DEPTH_COMPONENT,
                  GL_FLOAT, &z );

    // LOG( Logger::Debug, "Depth: " << z );

    glm::vec3 windowTarget = { (float) info.x,
                               (float) ( windowHeight_ - info.y ), (float) z };

    glm::vec4 viewport = { 0, 0, windowWidth_, windowHeight_ };
    glm::vec3 worldTarget =
        glm::unProjectNO( windowTarget, viewMatrix_, projMatrix_, viewport );

    // LOG( Logger::Debug, "Clicked at (" << worldTarget.x << ", " <<
    // worldTarget.y
    //                                    << ", " << worldTarget.z << ")" );

    if ( std::abs( worldTarget.z ) < 0.1f ) {
        if ( debugInteract ) {
            tycoon_->Debug( std::lround( worldTarget.x ),
                            std::lround( worldTarget.y ) );
        } else {
            tycoon_->Interact( std::lround( worldTarget.x ),
                               std::lround( worldTarget.y ) );
        }
    }
}

void Game::Private::CenterCamera( glm::vec2 center ) {
    // intialize view matrix
    viewMatrix_ = glm::mat4( 1.0 );
    viewMatrix_ = glm::translate(
        viewMatrix_, glm::vec3( -center.x, -center.y, -CAMERA_VIEW_DISTANCE ) );

    // static float time = 0.0f;
    // time += 1.0f / Globals::ticksPerSecond;

    // viewMatrix_ =
    //    glm::rotate( viewMatrix_, 0.02f * time, glm::vec3{ 0, 0, 1 } );

    combinedMatrix_ = projMatrix_ * viewMatrix_;
}

static Scm::Object ChangeFpsCap( Scm::Engine scheme, Scm::Object args ) {
    int newFps = Scm::Get< Scm::FixNum >( scheme.Car( args ) ).value;
    Globals::fpsCap = newFps;
    return scheme.Primitives.Void;
}

void Game::Private::InitScm() {
    LOG( Logger::Info, "Initializing scheme interface..." );

    scheme_.DefinePrimitiveProcedure( "fpscap", [ & ]( auto args ) {
        return ChangeFpsCap( scheme_, args );
    } );

    scheme_.DefinePrimitiveProcedure( "enable-vsync", [ & ]( auto args ) {
        glfwSwapInterval( 1 );
        return scheme_.Primitives.Void;
    } );

    scheme_.DefinePrimitiveProcedure( "disable-vsync", [ & ]( auto args ) {
        glfwSwapInterval( 0 );
        return scheme_.Primitives.Void;
    } );

    scheme_.DefinePrimitiveProcedure(
        "scream", [ & ]( auto args ) { return Scream( scheme_, args ); } );

    scheme_.DefinePrimitiveProcedure( "I", [ & ]( auto args ) {
        return Interact( scheme_, *tycoon_, args );
    } );

    scheme_.DefinePrimitiveProcedure( "debug", [ & ]( auto args ) {
        debugInteract ^= true;
        if ( debugInteract ) {
            LOG( Logger::Debug, "Debug interaction enabled." );
        } else {
            LOG( Logger::Debug, "Debug interaction disabled." );
        }
        return scheme_.Primitives.Void;
    } );
}

void Game::Private::Init() {
    LOG( Logger::Info, "Initializing game..." );

    // load a font
    font_ = &FindFont( "fonts/dejavu.fnt" );
    font_->scale_ = 1.0;

    codeFont_ = &FindFont( "fonts/code.fnt" );
    codeFont_->scale_ = 1.0;

    gui_ = std::make_unique< MainGui >();

    // gui_->pauseButton->Clicked.connect< &Game::Private::SayHi >( this );
    // gui_->storeButton->Clicked.connect< &Game::Private::SayShit >( this );

    tycoon_ = std::make_unique< Tycoon >();

    tycoon_->ConnectGui( *gui_ );

    liminal_ = std::make_unique< Liminal >();

    {
        // initialize shaders
        Shader shader = ShaderBuilder()
                            .Attach( "shaders/basic.vert", GL_VERTEX_SHADER )
                            .Attach( "shaders/basic.frag", GL_FRAGMENT_SHADER )
                            .Build();

        defaultShader_ = std::make_unique< Shader >( shader );
    }

    ShaderInfo info = {
        .id = *defaultShader_,
        .tint = defaultShader_->FindUniform( "uTint" ),
        .intensity = defaultShader_->FindUniform( "uIntensity" ),
        .alphaIntensity = defaultShader_->FindUniform( "uAlphaIntensity" ),
        .projMatrix = defaultShader_->FindUniform( "uProjMatrix" ),
        .viewMatrix = defaultShader_->FindUniform( "uViewMatrix" ),
        .viewPos = defaultShader_->FindUniform( "uViewPos" ),
        .time = defaultShader_->FindUniform( "uTime" ),
        .tex = defaultShader_->FindUniform( "uTex" ),
        .depthMap = defaultShader_->FindUniform( "uDepthMap" ),
    };

    defaultShader_->Use();

    glUniform1i( info.tex, 0 );
    glUniform1i( info.depthMap, 1 );

    batch_ = std::make_unique< Batch >( info );
    batch_->SetTint( { 1.0, 1.0, 1.0, 1.0 }, 1.0 );

    shadowMapper_ = std::make_unique< ShadowMapper >();

    // enable depth testing
    glEnable( GL_DEPTH_TEST );
    // but everyone always passes
    // glDepthFunc( GL_ALWAYS );
    
    glEnable( GL_CULL_FACE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    CenterCamera( { 0, 0 } );

    InitScm();
}

void Game::Private::ScmRun( const std::string & expr ) {
    try {
        LOG( Logger::Info, "[Scheme]: " << expr );

        auto x = scheme_.Read( expr );
        auto result = scheme_.Eval( x );

        std::cout << "[Scheme Result]: ";
        scheme_.Print( result );
        std::cout << std::endl;
    } catch ( Scm::Exception & e ) {
        LOG( Logger::Error, ( std::string( "[Scheme]: " ) + e.what() ) );
    }
}

void Game::Private::SayShit() {
    ScmRun( "(/ 1 0)" );
}

void Game::Private::SayHi() {
    ScmRun( "(print \"Hi there\")" );
}

void Game::Private::Tick( float timestep ) {
    // do some ticking
    tycoon_->Tick( timestep );
    liminal_->Tick( timestep );

    // CenterCamera( tycoon_->CameraFocus() );
    viewMatrix_ = liminal_->Camera();
    combinedMatrix_ = projMatrix_ * viewMatrix_;
}

void Game::Private::Render( float timestep ) {
    static float fpsTimer = 0.0;
    fpsTimer += timestep;

    if ( fpsTimer > 0.1 ) {
        fpsTimer = 0.0;
        gui_->fpsLabel->Text( std::to_string( (int) ceil( 1.0f / timestep ) ) );
    }

    for ( auto & timer : renderTimers ) {
        timer.Tick( timestep );
    }

    static float time = 0.0;
    time += timestep;

    defaultShader_->Use();

    glUniform1f( batch_->LocalShaderInfo().time, time );

    TickRegisteredTimers( timestep );

    gui_->MoveCursor( cursor );

    // static float elapsed = 0;
    // elapsed += timestep;
    // gui_->storeLayout->scrollerLayout->Scroll(5 * std::sin(elapsed) *
    // timestep);

    // bind shader

    // and load the matrices into it
    glUniformMatrix4fv( batch_->LocalShaderInfo().viewMatrix, 1, GL_FALSE,
                        glm::value_ptr( viewMatrix_ ) );

    glUniformMatrix4fv( batch_->LocalShaderInfo().projMatrix, 1, GL_FALSE,
                        glm::value_ptr( projMatrix_ ) );

    glUniform3fv( batch_->LocalShaderInfo().viewPos, 1,
                  glm::value_ptr( liminal_->CameraPos() ) );

    batch_->Begin( timestep );
    // just draw whatever the fuck we want
    // tycoon_->Render( batch_.get() );
    batch_->End();
    glDepthFunc( GL_LESS );

    shadowMapper_->SetLight( { 0.0, 2.0, 0.0 } );

    // render to shadow map
    shadowMapper_->Begin();

    batch_->Begin( timestep );
    liminal_->Render( *batch_ );
    batch_->End();

    shadowMapper_->End();

    // reset to default shader again
    defaultShader_->Use();

    // render normally with shadow mapping data
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, shadowMapper_->DepthTexture() );
    batch_->Begin( timestep );
    liminal_->Render( *batch_ );
    batch_->End();

    // and load the matrices into it
    glUniformMatrix4fv( batch_->LocalShaderInfo().viewMatrix, 1, GL_FALSE,
                        glm::value_ptr( guiCombinedMatrix_ ) );

    glUniformMatrix4fv( batch_->LocalShaderInfo().projMatrix, 1, GL_FALSE,
                        glm::value_ptr( glm::mat4( 1.0 ) ) );

    glDepthFunc( GL_ALWAYS );
    // batch_->Begin( timestep );

    // draw the gui
    // gui_->Draw( *batch_ );

    // batch_->SetTint( Colors::Green );
    // batch_->DrawQuad( cursor.x, cursor.y, 100, 100 );

    // batch_->End();
}

void Game::Private::Resize( int width, int height ) {
    // LOG(Loggers::INFO,
    //     "Resizing game to " << width << " x " << height << "...");

    // update internal values
    windowWidth_ = width;
    windowHeight_ = height;

    // notify opengl of the new size
    glViewport( 0, 0, width, height );

    // compute the projection matrix with this new info
    projMatrix_ = glm::perspective( glm::radians( 45.0f ),
                                    ( (float) width ) / ( (float) height ),
                                    0.1f, 100.0f );

    // update combined matrix
    combinedMatrix_ = projMatrix_ * viewMatrix_;

    guiCombinedMatrix_ =
        glm::ortho( 0.0f, (float) width, (float) height, 0.0f );
    gui_->Transform(
        { 10.0, 10.0, (float) ( width - 20.0 ), (float) ( height - 20.0 ) } );
}

Game::~Game() = default;

Game::Game( int width, int height ) {
    imp_ = std::make_unique< Game::Private >( width, height );
}

void Game::Tick( float timestep ) {
    imp_->Tick( timestep );
}

void Game::Render( float timestep ) {
    imp_->Render( timestep );
}

void Game::Resize( int width, int height ) {
    imp_->Resize( width, height );
}

void Game::MouseDown( CursorInfo info ) {
    bool handled = imp_->Gui()->MouseDown( info );
    if ( !handled ) {
        // propagate to tycoon / non-GUI stuff
        imp_->ViewMouseDown( info );
        imp_->liminal_->MouseDown( info );
    }
}

void Game::MouseUp( CursorInfo info ) {
    imp_->Gui()->MouseUp( info );
    imp_->liminal_->MouseUp( info );
}

void Game::MoveCursor( CursorInfo info ) {
    cursor = info;

    if ( !imp_->Gui()->paused ) {
        imp_->liminal_->MoveCursor( info );
    }
}

void Game::TextInput( char character ) {
    // LOG(Loggers::DEBUG, "Got a " << (char) character << ", woah");

    imp_->Gui()->TextInput( character );
    imp_->liminal_->TextInput( character );
}

void Game::Key( int key, int action ) {
    imp_->liminal_->Key( key, action );

    if ( action == GLFW_PRESS ) {
        imp_->Gui()->KeyDown( key );
    }
    if ( key == GLFW_KEY_ENTER && action == GLFW_PRESS ) {
        imp_->ScmRun( imp_->Gui()->consoleTextBox->Text() );
        imp_->Gui()->consoleTextBox->Text( "" );
    }
}
