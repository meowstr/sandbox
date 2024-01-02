#include "Game.h"

// #include "Font.h"
// #include "Material.h"
#include "GameMap.h"
#include "GameRegistry.h"
#include "GameState.h"
#include "Graphics.h"
#include "Logging.h"
#include "Math.h"
#include "Mesh.h"
#include "ObjReader.h"
#include "ResourceDirectory.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Tycoon.h"
#include "Utility.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace {

////////////////////////////////////////////////////////////////////////////////

glm::mat4 computeViewMatrix( state::GameState & state ) {
    // first compute orientation
    float adjustedPitch = -state.camera.pitch + glm::radians( 90.0f );
    float adjustedYaw = -state.camera.yaw + glm::radians( 180.0f );
    float adjustedRoll = -state.camera.roll + glm::radians( 90.0f );

    glm::vec3 forward{
        glm::sin( adjustedPitch ) * glm::sin( adjustedYaw ),
        glm::cos( adjustedPitch ),
        glm::sin( adjustedPitch ) * glm::cos( adjustedYaw ),
    };

    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right = glm::cross( forward, up );

    glm::vec3 newUp =
        glm::cos( adjustedRoll ) * right + glm::sin( adjustedRoll ) * up;

    // store intermediate vectors
    state.camera.forward = forward;
    state.camera.right = right;

    return glm::lookAt( state.camera.pos, state.camera.pos + forward, newUp );
}

////////////////////////////////////////////////////////////////////////////////

glm::vec3 toUiFromWorldPos( state::GameState & state, glm::vec3 pos ) {
    glm::mat4 toUiFromNative =
        glm::inverse( state.rendering.uiProjectionMatrix );
    glm::mat4 toNativeFromWorld =
        state.rendering.projectionMatrix * state.rendering.viewMatrix;

    glm::vec4 transformed =
        toUiFromNative * toNativeFromWorld * glm::vec4( pos, 1.0f );
    transformed = transformed / transformed.w;
    return glm::vec3( transformed.x, transformed.y, transformed.z );
}

////////////////////////////////////////////////////////////////////////////////

void updateCameraTransform( state::GameState & state ) {
    state.camera.pos = state.player.pos;
    // state.camera.pitch = state.player.pitch +
    // state.animations.cameraBobOffset;
    state.camera.pitch = state.player.pitch;
    state.camera.yaw = state.player.yaw;

    state.camera.pos.y += state.animations.cameraBobOffset * 3.0f;
}

////////////////////////////////////////////////////////////////////////////////

void tickEditMode( state::GameState & state ) {
    if ( state.editMode.editingEnabled ) {
        state::TransformEditInfo editInfo = state.editMode.editInfo;

        glm::mat4 & t = state.transforms[ editInfo.transformId ];

        glm::mat4 rotation = glm::rotate( glm::mat4( 1.0 ),
                                          (float) std::numbers::pi / 8.0f *
                                              editInfo.rotationTicks,
                                          glm::vec3( 0.0, 1.0, 0.0 ) );

        glm::mat4 delta =
            state.rendering.invViewMatrix * editInfo.initialViewTransform;

        glm::mat4 x = delta * editInfo.initialModelTransform;

        // rotate
        t = editInfo.initialModelTransform * rotation;

        // copy the translation vector
        t[ 3 ] = x[ 3 ];
    }
}

////////////////////////////////////////////////////////////////////////////////

void tickInput( state::GameState & state ) {
    glm::vec3 & pos = state.player.pos;
    int & states = state.input.states;

    const float walkingSpeed = 1.5f; // m/s

    float moveDelta = state.tickTimestep * walkingSpeed;

    if ( states & state::kInputRun ) {
        moveDelta *= state.player.runningMultiplier;
        state.player.running = true;
    } else {
        state.player.running = false;
    }

    int dx = 0;
    int dz = 0;

    if ( states & state::kInputRight ) {
        pos += moveDelta * state.player.right;
        dx++;
    }
    if ( states & state::kInputLeft ) {
        pos -= moveDelta * state.player.right;
        dx--;
    }
    if ( states & state::kInputUp ) {
        pos += moveDelta * state.player.forward;
        dz++;
    }
    if ( states & state::kInputDown ) {
        pos -= moveDelta * state.player.forward;
        dz--;
    }

    if ( dz != 0 ) {
        // we are moving
        state.animations.cameraBobEnable = state.settings.enableCameraBobbing;
    } else {
        // we are not moving
        state.animations.cameraBobEnable = false;
    }

    state.player.yaw = state.input.yaw;
    state.player.pitch = state.input.pitch;

    // DEBUG_LOG() << pos.x << ", " << pos.y << ", " << pos.y << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

void tickBobbingAnimation( state::GameState & state ) {
    state::Animations & animations = state.animations;

    animations.cameraBobTimer.setDuration(
        animations.cameraBobPeriod *
        ( state.player.running ? ( 1.0f / state.player.runningMultiplier )
                               : 1.0f ) );

    animations.cameraBobTimer.setLooping( animations.cameraBobEnable );
    animations.cameraBobTimer.tick( state.tickTimestep );

    float t = animations.cameraBobTimer.progress();
    animations.cameraBobOffset =
        -0.01f * std::abs( glm::sin( t * std::numbers::pi * 2.0f ) );
}

////////////////////////////////////////////////////////////////////////////////

void updatePlayerTransform( state::GameState & state ) {
    // compute forward to be perpendicular to y-axis
    float adjustedYaw = -state.player.yaw + glm::radians( 180.0f );
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    state.player.forward =
        glm::vec3( glm::sin( adjustedYaw ), 0.0f, glm::cos( adjustedYaw ) );
    state.player.right = glm::cross( state.player.forward, up );
}

////////////////////////////////////////////////////////////////////////////////

// void renderHightlight( engine::DefaultRenderer & renderer,
//                        const engine::Mesh & mesh ) {
//     renderer.flush();
//
//     glStencilOp( GL_KEEP, GL_REPLACE, GL_REPLACE );
//     glStencilFunc( GL_ALWAYS, 1, 0xff );
//     glStencilMask( 0xff );
//
//     renderer.render( mesh );
//     renderer.flush();
//
//     glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
//     glStencilMask( 0x00 );
//     glStencilFunc( GL_NOTEQUAL, 1, 0xff );
//
//     glDisable( GL_DEPTH_TEST );
//
//     renderer.push();
//
//     renderer.setShader( res::solidColorShader().defaultShader() );
//
//     renderer.applyTansform( glm::scale( glm::vec3( 1.01f ) ) );
//
//     engine::Material hightlight{ .tint = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f )
//     }; renderer.setMaterial( hightlight );
//
//     renderer.render( mesh );
//
//     renderer.pop();
//
//     glEnable( GL_DEPTH_TEST );
//
//     glStencilFunc( GL_ALWAYS, 1, 0xff );
// }

////////////////////////////////////////////////////////////////////////////////

template < typename S >
void readIfExists( const map::Map & map, const std::string & key,
                   typename S::Type & out ) {
    if ( auto value = map.queryTyped< S >( key ) ) {
        out = value.value();
        INFO_LOG() << "  OVERRIDE " << key << std::endl;
    } else {
        INFO_LOG() << "  DEFAULT  " << key << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////

void loadMap( state::GameState & state, std::filesystem::path path ) {
    map::Map map( path );

    using namespace map::serialize;

    INFO_LOG() << "Loading map:" << std::endl;

    // load map
    readIfExists< Float >( map, "player/runningMultiplier",
                           state.player.runningMultiplier );

    readIfExists< Bool >( map, "settings/enableCameraBobbing",
                          state.settings.enableCameraBobbing );

    std::vector< int > v;

    readIfExists< List< Int > >( map, "settings/enableCameraBobbing2", v );
}

////////////////////////////////////////////////////////////////////////////////

std::vector< glm::vec3 > verticesFromMesh( const engine::Mesh & mesh ) {
    std::vector< glm::vec3 > vs;

    for ( auto v : mesh.vertices ) {
        vs.push_back( v.position );
    }

    return vs;
}

////////////////////////////////////////////////////////////////////////////////

void setupConsoleLight( state::GameState & state ) {

    math::QuadData quadData = math::extractQuadData(
        verticesFromMesh( state.rendering.models.computerScreen ) );

    glm::vec3 normal =
        state.rendering.models.computerScreen.vertices[ 0 ].normal;
    const float nearDist = 0.3f; // in model coords
    const float farDist = 4.0f;  // in model coords

    glm::mat4 viewMat = glm::lookAt( quadData.center - normal * nearDist,
                                     quadData.center, quadData.up );
    glm::mat4 frustumMat = glm::frustum(
        quadData.width * -0.5f, quadData.width * 0.5f, quadData.height * -0.5f,
        quadData.height * 0.5f, nearDist, farDist );

    state.rendering.light.transform = frustumMat * viewMat;
    state.rendering.light.direction = normal;
}

////////////////////////////////////////////////////////////////////////////////

void initRendering( state::GameState & state ) {
    state::Rendering & rendering = state.rendering;

    rendering.pixelelizeFactor = 8;

    state::DefaultShader & defaultShader = rendering.defaultShader;
    state::IdentityShader & identityShader = rendering.identityShader;

    // load shader
    gfx::ShaderBuilder()
        .attach( res::directory() / "shaders/default.vert", GL_VERTEX_SHADER )
        .attach( res::directory() / "shaders/default.frag", GL_FRAGMENT_SHADER )
        .build( defaultShader.program );

    defaultShader.uProj = gfx::findUniform( defaultShader.program, "uProj" );
    defaultShader.uView = gfx::findUniform( defaultShader.program, "uView" );
    defaultShader.uTint = gfx::findUniform( defaultShader.program, "uTint" );
    defaultShader.uUseTexture =
        gfx::findUniform( defaultShader.program, "uUseTexture" );
    defaultShader.uTexture =
        gfx::findUniform( defaultShader.program, "uTexture" );
    defaultShader.uEyePos =
        gfx::findUniform( defaultShader.program, "uEyePos" );
    defaultShader.uLightTransform =
        gfx::findUniform( defaultShader.program, "uLightTransform" );
    defaultShader.uLightTexture =
        gfx::findUniform( defaultShader.program, "uLightTexture" );
    defaultShader.uLightDirection =
        gfx::findUniform( defaultShader.program, "uLightDirection" );

    gfx::ShaderBuilder()
        .attach( res::directory() / "shaders/identity.vert", GL_VERTEX_SHADER )
        .attach( res::directory() / "shaders/identity.frag",
                 GL_FRAGMENT_SHADER )
        .build( identityShader.program );

    identityShader.uTexture =
        gfx::findUniform( identityShader.program, "uTexture" );

    using namespace engine::objReader;

    state::Textures & textures = rendering.textures;

    engine::loadTexture( textures.jahy,
                         res::directory() / "textures/psim.png" );

    state::Models & models = rendering.models;

    loadMesh( models.cube, res::directory() / "models/cube.obj" );
    loadMesh( models.room, res::directory() / "models/emptyroom2.obj" );

    engine::Mesh computerMesh;
    ObjFileProperties computerProps;

    loadMesh( computerMesh, res::directory() / "models/computer.obj",
              &computerProps );

    queryObjMesh( models.computer, computerMesh, computerProps, "Cube" );
    queryObjMesh( models.computerScreen, computerMesh, computerProps,
                  "Screen" );

    // native coordinates
    ERROR_LOG() << "FIXME FIRST" << std::endl;
    // utils::meshes::quad2DTopLeft( models.screenQuad, { -1, -1 }, { 2, 2 } );

    gfx::initBuffer( rendering.defaultBuffer );
    gfx::initBuffer( rendering.computerBuffer );
    gfx::initBuffer( rendering.computerScreenBuffer );
    gfx::initBuffer( rendering.screenQuadBuffer );

    gfx::writeBuffer( rendering.defaultBuffer, models.room.vertices );
    gfx::writeBuffer( rendering.computerBuffer, models.computer.vertices );
    gfx::writeBuffer( rendering.computerScreenBuffer,
                      models.computerScreen.vertices );
    gfx::writeBuffer( rendering.screenQuadBuffer, models.screenQuad.vertices );

    setupConsoleLight( state );
}

////////////////////////////////////////////////////////////////////////////////

void renderScene( state::GameState & state ) {

    auto & rendering = state.rendering;
    auto & shader = state.rendering.defaultShader;

    auto combined = rendering.projectionMatrix * rendering.viewMatrix;

    glUseProgram( shader.program );

    gfx::setUniform( shader.uProj, combined );
    gfx::setUniform( shader.uView, glm::mat4{ 1.0 } );

    gfx::setUniform( shader.uLightTransform,
                     rendering.light.transform *
                         glm::inverse( state.transforms[ 1 ] ) );
    gfx::setUniform( shader.uLightTexture, 1 );

    glm::vec4 lightDirection =
        state.transforms[ 1 ] * glm::vec4( rendering.light.direction, 0.0f );
    gfx::setUniform(
        shader.uLightDirection,
        glm::vec3( lightDirection.x, lightDirection.y, lightDirection.z ) );

    gfx::setUniform( shader.uEyePos, state.camera.pos );
    gfx::setUniform( shader.uTexture, 0 );
    gfx::setUniform( shader.uUseTexture, 0 );
    gfx::setUniform( shader.uTint, glm::vec4{ 0.5f, 0.5f, 0.5f, 1.0f } );

    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, rendering.textures.jahy );

    // render room buffer
    gfx::setUniform( shader.uView, state.transforms[ 0 ] );
    gfx::draw( state.rendering.defaultBuffer );

    gfx::setUniform( shader.uView, state.transforms[ 1 ] );
    gfx::draw( state.rendering.computerBuffer );

    // render texture to screen
    // glBindTexture( GL_TEXTURE_2D, rendering.textures.jahy );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, rendering.textures.jahy );
    gfx::setUniform( shader.uTexture, 0 );
    gfx::setUniform( shader.uUseTexture, 1 );
    gfx::draw( state.rendering.computerScreenBuffer );
}

////////////////////////////////////////////////////////////////////////////////

void renderSceneUi( state::GameState & state ) {
    auto & rendering = state.rendering;
    auto & shader = state.rendering.defaultShader;

    // glUseProgram( shader.program );

    // gfx::setUniform( shader.uProj, state.rendering.uiProjectionMatrix );
    // gfx::setUniform( shader.uView, glm::mat4{ 1.0 } );
    // gfx::setUniform( shader.uEyePos, state.camera.pos );
    // gfx::setUniform( shader.uTexture, 0 );
    // gfx::setUniform( shader.uUseTexture, 0 );
    // gfx::setUniform( shader.uTint, glm::vec4{ 0.5f, 0.5f, 0.5f, 1.0f } );
}

////////////////////////////////////////////////////////////////////////////////

void updateScreenSize( state::GameState & state, int width, int height ) {
    state::Rendering & rendering = state.rendering;

    rendering.renderWidth = width;
    rendering.renderHeight = height;
    rendering.subRenderWidth = width / rendering.pixelelizeFactor;
    rendering.subRenderHeight = height / rendering.pixelelizeFactor;

    engine::genTexture( rendering.framebuffers.frambufferTexture1,
                        rendering.subRenderWidth, rendering.subRenderHeight );

    engine::genTexture( rendering.textures.lastFrame, rendering.subRenderWidth,
                        rendering.subRenderHeight );

    engine::genDepthStencilTexture(
        rendering.framebuffers.frambufferDepthTexture1,
        rendering.subRenderWidth, rendering.subRenderHeight );

    // set texture on framebuffer
    glBindFramebuffer( GL_FRAMEBUFFER, rendering.framebuffers.framebuffer1 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            rendering.framebuffers.frambufferTexture1, 0 );

    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_TEXTURE_2D,
                            rendering.framebuffers.frambufferDepthTexture1, 0 );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

////////////////////////////////////////////////////////////////////////////////

void startEditingTransform( state::GameState & state ) {
    // TODO: make this independent of number of viewports
    state::TransformEditInfo & editInfo = state.editMode.editInfo;
    int id = 1;
    editInfo.transformId = id;
    editInfo.rotationTicks = 0;
    editInfo.initialModelTransform = state.transforms[ id ];
    editInfo.initialViewTransform = state.rendering.viewMatrix;
}

////////////////////////////////////////////////////////////////////////////////

void stopEditingTransform( state::GameState & state ) {
    // do nothing
}

void cancelEditingTransform( state::GameState & state ) {
    state::TransformEditInfo & editInfo = state.editMode.editInfo;
    state.transforms[ editInfo.transformId ] = editInfo.initialModelTransform;
    state.editMode.editingEnabled = false;
}

void handleInputResize( state::GameState & state, int width, int height ) {
    tycoon::InputEventData inputData;
    inputData.size[ 0 ] = (int) width;
    inputData.size[ 1 ] = (int) height;
    tycoon::input( state, tycoon::InputEvent::Resize, inputData );
    return;

    updateScreenSize( state, (int) width, (int) height );
}

// void toggleEditTransform() override {
//     return;
//     mState.editMode.editingEnabled = !mState.editMode.editingEnabled;
//
//     if ( mState.editMode.editingEnabled ) {
//         startEditingTransform( mState );
//     } else {
//         stopEditingTransform( mState );
//     }
// }
//
// void cancelEditTransform() override {
//     return;
//     cancelEditingTransform( mState );
// }
//
// void rotateEditTransform( int x ) override {
//     return;
//     mState.editMode.editInfo.rotationTicks += x;
// }
//
// void setInputLag( float lag ) override {
//     mState.inputLag = lag;
// }

////////////////////////////////////////////////////////////////////////////////

void initGame( state::GameState & state ) {
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    state.player.pos = glm::vec3{ 0.0f, 1.6f, 0.0f };
    state.player.yaw = 0.0f;
    state.player.pitch = 0.0f;
    state.player.running = false;
    state.player.runningMultiplier = 2.0f;

    state.camera.pos = glm::vec3();
    state.camera.yaw = 0.0f;
    state.camera.pitch = 0.0f;
    state.camera.roll = 0.0f;

    state.animations.cameraBobEnable = true;
    state.animations.cameraBobTimer = utils::ManualTimer( 1.0f );
    state.animations.cameraBobTimer.tick( 10.0 );
    state.animations.cameraBobOffset = 0.0f;
    state.animations.cameraBobPeriod = 1.0f;

    state.settings.enableCameraBobbing = true;

    for ( int i = 0; i < 2; i++ ) {
        state.transforms.push_back( glm::mat4( 1.0 ) );
    }

    state.editMode.editingEnabled = false;

    loadMap( state, res::directory() / "maps/map1.map" );

    initRendering( state );

    updateCameraTransform( state );
}

////////////////////////////////////////////////////////////////////////////////

void tickGame( state::GameState & state, float timestep ) {
    state.tickTimestep = timestep;

    tickInput( state );
    tickBobbingAnimation( state );

    updatePlayerTransform( state );
    updateCameraTransform( state );
}

////////////////////////////////////////////////////////////////////////////////

void renderGame( state::GameState & state, Shiro shiro, float timestep ) {
    // glEnable( GL_DEPTH_TEST );
    // glEnable( GL_STENCIL_TEST );
    // glStencilMask( 0x00 );

    state.renderTimestep = timestep;

    // apply an internal view matrix
    auto viewMatrix = computeViewMatrix( state );

    state.rendering.projectionMatrix = shiro.projectionMatrix;
    state.rendering.viewMatrix = shiro.viewMatrix * viewMatrix;
    state.rendering.invViewMatrix = glm::inverse( state.rendering.viewMatrix );

    tickEditMode( state );

    glBindFramebuffer( GL_FRAMEBUFFER,
                       state.rendering.framebuffers.framebuffer1 );

    glViewport( 0, 0, state.rendering.subRenderWidth,
                state.rendering.subRenderHeight );

    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    renderScene( state );

    // copy frame to another texture
    {
        glReadBuffer( GL_COLOR_ATTACHMENT0 );

        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, state.rendering.textures.lastFrame );

        glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                             state.rendering.subRenderWidth,
                             state.rendering.subRenderHeight );
    }

    glViewport( 0, 0, state.rendering.renderWidth,
                state.rendering.renderHeight );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    auto & idShader = state.rendering.identityShader;

    glUseProgram( idShader.program );

    // glBindTexture( GL_TEXTURE_2D, state.rendering.textures.jahy );
    glBindTexture( GL_TEXTURE_2D,
                   state.rendering.framebuffers.frambufferTexture1 );
    gfx::setUniform( idShader.uTexture, 0 );

    gfx::draw( state.rendering.screenQuadBuffer );

    // renderer.applyTansform( viewMatrix );

    // apply shiro matrices (ones that represent the final image view)
    // renderer.setProjection( shiro.projectionMatrix * shiro.viewMatrix *
    //                        viewMatrix );

    {
        // static const std::unique_ptr< engine::Mesh > room =
        //     engine::objReader::loadMesh( res::directory() /
        //                                  "models/emptyroom.obj" );

        // engine::Material material{ .tint = glm::vec4( 0.3, 0.3, 0.3, 1.0 ) };
        // renderer.setMaterial( material );

        // renderer.render( *room );
        //  renderHightlight( renderer, *room );
    }

    // render a test quad
    {
        // static const std::unique_ptr< engine::Mesh > quad =
        //     utils::meshes::quad2D( { 0.0f, 0.0f }, { 0.4f, 0.4f } );

        // static const std::unique_ptr< engine::Mesh > cube =
        //     engine::objReader::loadMesh( res::directory() / "models/cube.obj"
        //     );

        // static const auto tex =
        //     res::texture( res::directory() / "textures/jahy.jpg" );

        // renderer.push();

        // renderer.applyTansform(
        //     glm::translate( glm::vec3{ 0.0f, 0.0f, -5.0f } ) );
        // renderer.applyTansform(
        //     glm::rotate( mImp->mTime, glm::vec3{ 0.0f, 1.0f, 0.0f } ) );

        // engine::Material quadMaterial{ .tint = glm::vec4( 0.5, 0.5, 0.5, 1.0
        // ),
        //                                .texture = tex };
        // renderer.setMaterial( quadMaterial );

        // renderHightlight( renderer, *cube );
        //// renderer.render( *cube );

        // renderer.pop();
    }

    // renderer.pop();

    // make sure to flush renderer when done
    // renderer.flush();

    // glStencilMask( 0xff );
}

namespace gameImpl {

////////////////////////////////////////////////////////////////////////////////

void init( state::GameState & state ) {
    tycoon::init( state );
}

////////////////////////////////////////////////////////////////////////////////

void input( state::GameState & state, game::InputEvent e,
            game::InputEventData data ) {
    tycoon::InputEventData eData;
    eData.pos[ 0 ] = state.shared.mouseCursor[ 0 ];
    eData.pos[ 1 ] = state.shared.mouseCursor[ 1 ];
    switch ( e ) {
    case game::InputEvent::kClick:
        tycoon::input( state, tycoon::InputEvent::Click, eData );
        break;
    case game::InputEvent::kResize:
        handleInputResize( state, (int) data.size[ 0 ], (int) data.size[ 1 ] );
        break;
    case game::InputEvent::kRotate:
        tycoon::input( state, tycoon::InputEvent::RotateTool, eData );
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////

void tick( state::GameState & state, float timestep ) {
    state.tickTimestep = timestep;
    // tickGame( state, *mImp->mController, timestep );

    // tycoon::InputEventData eData;
    // eData.pos[ 0 ] = (int) mImp->mController->mouseX();
    // eData.pos[ 1 ] = (int) mImp->mController->mouseY();
    // tycoon::input( state, tycoon::InputEvent::Hover, eData );

    //// TODO: use glfw callback to gather inputs (possibly several per frame)
    // static bool wasMouseDown = false;
    // bool mouseDown = mImp->mController->mouseDown();
    // if ( mouseDown && !wasMouseDown ) {
    //     tycoon::input( state, tycoon::InputEvent::Click, eData );
    // }
    // wasMouseDown = mouseDown;

    tycoon::tick( state );
}

////////////////////////////////////////////////////////////////////////////////

void render( state::GameState & state, Shiro shiro, float timestep ) {
    state.renderTimestep = timestep;

    // renderGame( state, shiro, timestep );
    tycoon::render( state );
}

////////////////////////////////////////////////////////////////////////////////

void renderUi( state::GameState & state, Shiro shiro, float timestep ) {

    // glDisable( GL_DEPTH_TEST );

    state.rendering.uiProjectionMatrix = shiro.projectionMatrix;

    renderSceneUi( state );

    // apply shiro
    // renderer.setProjection( shiro.projectionMatrix );

    {
        // static const std::unique_ptr< engine::Mesh > quad =
        //     utils::meshes::quad2D( { 0.0f, 0.0f }, { 10.0f, 10.0f } );

        // renderer.push();

        // glm::vec3 pos =
        //     toUiFromWorldPos( state, glm::vec3{ 0.0f, 2.0f, -5.0f } );

        // renderer.applyTansform( glm::translate( pos ) );

        // engine::Material quadMaterial{
        //     .tint =
        //         glm::vec4( 255.0 / 255.0, 99.0 / 255.0, 71.0 / 255.0, 1.0 )
        //         };
        // renderer.setMaterial( quadMaterial );

        // renderer.render( *quad );

        // renderer.pop();
    }

    {
        // static auto font = res::font( res::directory() / "fonts/code.fnt" );

        // font->setLineHeight( 20.0f );

        // glm::vec3 pos =
        //     toUiFromWorldPos( state, glm::vec3{ 0.0f, 2.0f, -5.0f } );

        // float width = font->textWidth( "nyaa~ uwu" );
        // engine::renderText( renderer, *font, "nyaa~ uwu",
        //                     { pos.x - width * 0.5f, pos.y - 30.0f, pos.z } );

        // static utils::ManualTimer timer( 0.1f );
        // static std::string fpsString;

        // timer.tick( timestep );
        // if ( timer.isFinished() ) {
        //     fpsString = std::to_string( (int) std::round( 1.0f / timestep )
        //     ); timer.start();
        // }

        // engine::renderText( renderer, *font, fpsString,
        //{ 10.0f, 10.0f, 0.0f } );
    }
}

} // namespace gameImpl

} // namespace

struct GameImpl : public game::IGame {

    virtual state::GameState * allocateState() {
        return new state::GameState;
    }

    virtual void deallocateState( state::GameState * state ) {
        delete state;
    }

    virtual state::SharedState & sharedState( state::GameState & state ) {
        return state.shared;
    }

    virtual void init( state::GameState & state ) {
        gameImpl::init( state );
    }

    virtual void input( state::GameState & state, game::InputEvent e,
                        game::InputEventData data ) {
        gameImpl::input( state, e, data );
    }

    /// @param timestep the time step of the current tick
    virtual void tick( state::GameState & state, float timestep ) {
        gameImpl::tick( state, timestep );
    }

    /// @param shiro the transforms to apply at the end of the transform stack
    /// @param timestep the time step of the current frame
    virtual void render( state::GameState & state, Shiro shiro,
                         float timestep ) {
        gameImpl::render( state, shiro, timestep );
    }

    /// @param shiro the transforms to apply at the end of the transform stack
    /// @param timestep the time step of the current frame
    virtual void renderUi( state::GameState & state, Shiro shiro,
                           float timestep ) {
        gameImpl::renderUi( state, shiro, timestep );
    }
};

// register game
namespace {
static GameImpl kGame;
}

extern "C" void initSharedLibrary() {
    game::bindFunctions( &kGame );
}
