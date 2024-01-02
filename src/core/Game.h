#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <memory>

#include "SharedState.h"

struct Shiro {
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

namespace state {
struct GameState;
};

namespace game {

enum class InputEvent {
    kClick,
    kResize,
    kRotate,
};

union InputEventData {
    int pos[ 2 ];
    int size[ 2 ];
};

/// Class used for dynamically binding to a game through an interface
/// NOTE: don't be scared, this isn't an OOP construct. This is syntactic sugar
/// for building a vtable that can dynamically dispatch to our game functions.
/// Useful for hot-swapping
struct IGame {
    virtual state::GameState * allocateState() = 0;

    virtual void deallocateState( state::GameState * state ) = 0;

    virtual state::SharedState & sharedState( state::GameState & state ) = 0;

    virtual void init( state::GameState & state ) = 0;

    virtual void input( state::GameState & state, InputEvent e,
                        InputEventData data ) = 0;

    /// @param timestep the time step of the current tick
    virtual void tick( state::GameState & state, float timestep ) = 0;

    /// @param shiro the transforms to apply at the end of the transform stack
    /// @param timestep the time step of the current frame
    virtual void render( state::GameState & state, Shiro shiro,
                         float timestep ) = 0;

    /// @param shiro the transforms to apply at the end of the transform stack
    /// @param timestep the time step of the current frame
    virtual void renderUi( state::GameState & state, Shiro shiro,
                           float timestep ) = 0;
};

} // namespace game
