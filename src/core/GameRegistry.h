#pragma once

#include "Game.h"

namespace game {

void loadGameBinds( std::string name );
void unloadGameBinds();

/// The global game interface
IGame & binds();

void bindFunctions( IGame * game );
void unbindFunctions();

} // namespace game
