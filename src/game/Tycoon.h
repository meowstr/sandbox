#pragma once

#include "GameState.h"

namespace tycoon {

enum class InputEvent {
    Click,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    RotateTool,
    Resize,
};

union InputEventData {
    int pos[ 2 ];
    int size[ 2 ];
    int delta;
};

void init( state::GameState & state );
void input( state::GameState & state, InputEvent e, InputEventData data );
void tick( state::GameState & state );
void render( state::GameState & state );

} // namespace tycoon
