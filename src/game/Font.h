#pragma once

#include "GlRaii.h"

#include <memory>
#include <string>
#include <vector>

#define MAX_CHARS 256

namespace engine {

// TODO: there is memory to be saved here

struct Character {
    // valid flag for empty chars
    bool valid;

    int x;
    int y;
    int width;
    int height;
    int xOffset;
    int yOffset;
    int xAdvance;

    int8_t kernings[ MAX_CHARS ];
};

// struct ColorRun {
//     // color
//     Color color;
//     // number of character to color for
//     int length;
// };

// TODO: make this a class and everything in it private
// TODO: idk actually *sad pog*
struct BitmapFont {
    std::string name;
    std::string textureFilename;

    gl::Texture texture;

    int textureWidth;
    int textureHeight;

    int size;
    int lineHeight;
    int base;
    int charCount;

    Character chars[ MAX_CHARS ];
};

struct Glyph {
    // texture region
    float u1; // bottom-left
    float v1; // bottom left
    float u2; // top-right
    float v2; // top-right

    // render region
    float x; // top-left
    float y; // top-left
    float w; // width
    float h; // height
};

struct TextSize {
    // bounding box
    float width;
    float height;
};

void loadFont( BitmapFont & font, const std::string & path );

/// @returns text width in the font's native scaling
float textWidth( const BitmapFont & font, const std::string & text );

/// Computes glyph positions for a line of text. The top-right of the text is
/// (0, 0), and the text is positioned with the font's native scaling.
TextSize glyphs( std::vector< Glyph > & oGlyphs, const BitmapFont & font,
                 const std::string & text );

} // namespace engine
