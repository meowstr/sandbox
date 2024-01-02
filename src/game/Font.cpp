#include "Font.h"

#include "Logging.h"
#include "Texture.h"

#include <fstream>
#include <map>
#include <regex>
#include <string>

namespace engine {

void loadFont( BitmapFont & font, const std::string & path ) {
    INFO_LOG() << "Loading font " << path << "." << std::endl;

    std::ifstream file( path );

    // clear out the character array
    Character blank;
    blank.valid = false;
    for ( int i = 0; i < MAX_CHARS; i++ ) {
        font.chars[ i ] = blank;
    }

    // parse line 1
    {
        std::string line;
        std::getline( file, line );

        std::regex regex( R"%(^info face="([^"]*)" size=([0-9]+).*)%" );
        std::smatch m;

        if ( !std::regex_match( line, m, regex ) ) {
            throw "Failed to match.";
        }

        font.name = m[ 1 ];
        font.size = std::stoi( m[ 2 ] );
    }

    // parse line 2
    {
        std::string line;
        std::getline( file, line );

        std::regex regex(
            R"%(^common lineHeight=([0-9]+) base=([0-9]+) scaleW=([0-9]+) scaleH=([0-9]+).*)%" );
        std::smatch m;

        if ( !std::regex_match( line, m, regex ) ) {
            throw "Failed to match.";
        }

        font.lineHeight = std::stoi( m[ 1 ] );
        font.base = std::stoi( m[ 2 ] );
        font.textureWidth = std::stoi( m[ 3 ] );
        font.textureHeight = std::stoi( m[ 4 ] );
    }

    // parse line 3
    {
        std::string line;
        std::getline( file, line );

        std::regex regex( R"%(^page id=[0-9]+ file="([^"]*)".*)%" );
        std::smatch m;

        if ( !std::regex_match( line, m, regex ) ) {
            throw "Failed to match.";
        }

        font.textureFilename = m[ 1 ];
    }

    // parse character header
    {
        std::string line;
        std::getline( file, line );

        std::regex regex( R"%(^chars count=([0-9]+).*)%" );
        std::smatch m;

        if ( !std::regex_match( line, m, regex ) ) {
            throw "Failed to match.";
        }

        font.charCount = std::stoi( m[ 1 ] );
    }

    // parse characters
    for ( int i = 0; i < font.charCount; i++ ) {
        std::string line;
        std::getline( file, line );

        std::regex regex(
            R"%(^char id=([0-9]+)\s* x=([0-9]+)\s* y=([0-9]+)\s* width=([0-9]+)\s* height=([0-9]+)\s* xoffset=([-+]?[0-9]+)\s* yoffset=([-+]?[0-9]+)\s* xadvance=([0-9]+).*)%" );
        std::smatch m;

        if ( !std::regex_match( line, m, regex ) ) {
            throw std::runtime_error(
                "Font file failed to match: expected char line." );
        }

        int id = std::stoi( m[ 1 ] );

        Character ch = {
            .valid = true,
            .x = std::stoi( m[ 2 ] ),
            .y = std::stoi( m[ 3 ] ),
            .width = std::stoi( m[ 4 ] ),
            .height = std::stoi( m[ 5 ] ),
            .xOffset = std::stoi( m[ 6 ] ),
            .yOffset = std::stoi( m[ 7 ] ),
            .xAdvance = std::stoi( m[ 8 ] ),
        };

        std::fill( ch.kernings, ch.kernings + MAX_CHARS, 0 );

        // LOG(Loggers::DEBUG,
        //     "Parsed character: " << ch.x_ << " " << ch.y_ << " " << ch.width_
        //                          << " " << ch.height_ << " " << ch.xOffset_
        //                          << " " << ch.yOffset_ << " " <<
        //                          ch.xAdvance_);

        font.chars[ id ] = ch;
    }

    int kerningsCount;

    // parse kerning header line
    {
        std::string line;
        std::getline( file, line );

        std::regex regex( R"%(^kernings count=([0-9]+).*)%" );
        std::smatch m;

        if ( !std::regex_match( line, m, regex ) ) {
            // LOG(Loggers::ERROR, "No kernings header.");
            // throw "Failed to match.";
            kerningsCount = 0;
        } else {
            kerningsCount = std::stoi( m[ 1 ] );
        }
    }

    // parse kernings
    for ( int i = 0; i < kerningsCount; i++ ) {
        std::string line;
        std::getline( file, line );

        std::regex regex(
            R"%(^kerning first=([0-9]+)\s* second=([0-9]+)\s* amount=([-+]?[0-9]+).*)%" );
        std::smatch m;

        if ( !std::regex_match( line, m, regex ) ) {
            throw std::runtime_error(
                "Font file failed to match: expected kerning line." );
        }

        int first = std::stoi( m[ 1 ] );
        int second = std::stoi( m[ 2 ] );
        int amount = std::stoi( m[ 3 ] );

        // LOG(Loggers::DEBUG,
        //     "Parsed kerning: " << first << " " << second << " " << amount);

        // store in character
        font.chars[ first ].kernings[ second ] = amount;
    }

    // load texture
    auto x = path.find_last_of( '/' );
    std::string texturePath;
    if ( x >= 0 ) {
        texturePath = path.substr( 0, x + 1 ) + font.textureFilename;
    } else {
        texturePath = font.textureFilename;
    }

    font.texture.init();
    loadTexture( font.texture, texturePath );
}

float textWidth( const BitmapFont & font, const std::string & text ) {
    float cursorX = 0;

    const Character * lastData = nullptr;
    for ( char c : text ) {
        const Character & data = font.chars[ c ];

        cursorX += data.xAdvance;

        if ( lastData ) {
            cursorX += lastData->kernings[ c ];
        }

        lastData = font.chars + c;
    }

    return cursorX;
}

TextSize glyphs( std::vector< Glyph > & oGlyphs, const BitmapFont & font,
                 const std::string & text ) {

    float cursorX = 0.0f;
    float cursorY = 0.0f;

    // float textWidth = cursorX;
    // float textHeight = cursorY + font.lineHeight;

    float textWidth = 0.0f;
    float textHeight = 0.0f;

    const Character * lastCharData = nullptr;

    for ( char c : text ) {

        if ( c == '\n' ) {
            cursorY += font.lineHeight;
            cursorX = 0.0f;
            continue;
        }

        // if ( !isgraph( c ) ) {
        //     continue;
        // }

        const Character & charData = font.chars[ c ];

        Glyph glyph;

        // float ex = 0.5f / font.textureWidth;
        // float ey = 0.5f / font.textureHeight;

        // bottom left
        glyph.u1 = (float) ( charData.x ) / font.textureWidth;
        glyph.v1 =
            (float) ( charData.y + charData.height ) / font.textureHeight;
        // top right
        glyph.u2 = (float) ( charData.x + charData.width ) / font.textureWidth;
        glyph.v2 = (float) ( charData.y ) / font.textureHeight;

        // if there was a character before us
        if ( lastCharData ) {
            // move by kerning amount
            cursorX += lastCharData->kernings[ c ];
        }

        // top-left
        glyph.x = cursorX + charData.xOffset;
        glyph.y = cursorY + charData.yOffset;
        glyph.w = charData.width;
        glyph.h = charData.height;

        oGlyphs.push_back( glyph );

        // advance
        cursorX += charData.xAdvance;

        // keep track for kerning purposes
        lastCharData = font.chars + c;

        textWidth = std::max( textWidth, cursorX );
        textHeight = cursorY + font.lineHeight;
    }

    return { textWidth, textHeight };
}

// float BitmapFont::Draw( Renderer & renderer, const std::string & text, float
// x,
//                         float y, const std::vector< ColorRun > * runs ) {
//     float cursorX = x;
//     float cursorY = y;
//
//     Character * lastCharData = nullptr;
//
//     renderer.SetTexture( *texture_ );
//
//     int runCounter = 0;
//
//     // im sorry, i didn't want to type that
//     decltype( runs->begin() ) run;
//
//     if ( runs ) {
//         run = runs->begin();
//         if ( run != runs->end() ) {
//             renderer.Tint( run->color );
//         }
//     }
//
//     for ( char c : text ) {
//         Character & charData = chars_[ c ];
//
//         // bottom left
//         float u = (float) ( charData.x_ ) / textureWidth_;
//         float v = (float) ( charData.y_ + charData.height_ ) /
//         textureHeight_;
//         // top right
//         float s = (float) ( charData.x_ + charData.width_ ) / textureWidth_;
//         float t = (float) ( charData.y_ ) / textureHeight_;
//
//         // if there was a character before us
//         if ( lastCharData ) {
//             // move by kerning amount
//             cursorX += lastCharData->kernings_[ c ] * scale_;
//         }
//
//         // draw the char
//         renderer.DrawQuad2D( { cursorX + ( charData.xOffset_ * scale_ ),
//                                cursorY + ( charData.yOffset_ * scale_ ),
//                                charData.width_ * scale_,
//                                charData.height_ * scale_ },
//                              { u, v, s, t } );
//
//         // advance
//         cursorX += charData.xAdvance_ * scale_;
//
//         // keep track for kerning purposes
//         lastCharData = chars_ + c;
//
//         // swap text color if need be
//         if ( runs ) {
//             runCounter++;
//             // swap colors
//             if ( run != runs->end() && runCounter >= run->length ) {
//                 // choose next run
//                 run++;
//                 if ( run != runs->end() ) {
//                     renderer.Tint( run->color.Vec4() );
//                 }
//                 runCounter = 0;
//             }
//         }
//     }
//
//     // batch.Flush();
//
//     return cursorX - x;
// }

} // namespace engine
