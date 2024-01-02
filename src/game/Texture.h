#pragma once

#include "GlRaii.h"

#include <filesystem>
#include <memory>

namespace engine {

void loadTexture( gl::Texture & texture, const std::filesystem::path & path );

void genTexture( gl::Texture & texture, int width, int height );

void genDepthStencilTexture( gl::Texture & texture, int width, int height );

} // namespace engine
