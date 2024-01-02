#version 330 core

in vec2 gTextureCoords;

out vec4 gFragColor;

uniform sampler2D uTexture;
uniform float uGlowFactor;
uniform float uWipeHeight;

// const float kCathodeWeight[ 8 ] =
//     float[]( 0.3f, 0.8f, 0.9f, 1.0f, 1.0f, 0.9f, 0.8f, 0.1f );
//
// const float kCathodeXWeight[ 2 ] = float[]( 1.0f, 0.2f );

void main() {
    // apply wipe
    float wipeFactor = clamp(
        0.01f / ( gTextureCoords.y - ( 1.0f - uWipeHeight ) ), 0.0f, 1.0f );

    // vec2 coords =
    //     vec2( gTextureCoords.x + wipeFactor * 0.025f *
    //                                  fract( sin( gTextureCoords.y * 5821.0f )
    //                                  ),
    //           gTextureCoords.y );
    vec2 coords =
        vec2( gTextureCoords.x + wipeFactor * 0.0025f, gTextureCoords.y );

    vec2 uv = coords;

    vec4 input = texture( uTexture, uv );

    vec3 color = input.rgb;

    // float lum = ( 0.2126f * input.r + 0.7152f * input.g + 0.0722f * input.b
    // );
    float lum = ( 0.333f * input.r + 0.333f * input.g + 0.333f * input.b );

    // apply glow
    color *= uGlowFactor;

    // color *= ( 0.8f * wipeFactor + 0.2f );

    // if ( ( int( gl_FragCoord.x ) + 1 ) % 2 < 1 ) {
    //     color = vec3( 0.0f, 0.0f, 0.0f );
    // }

    float lumFactor = 0.5f + wipeFactor;
    // int cathodeX = ( int( gl_FragCoord.x ) + 1 ) % 2;
    //int cathodeXWeight = ( ( int( gl_FragCoord.x ) + 1 ) % 8 > 0 ) ? 1 : 0;

    // temp disable lum
    lum = 1.0f;
    //lumFactor = 1.0f;

    int cathodeXWeight = 1;
    color = mix( vec3( 0.0f, 0.0f, 0.0f ), color,
                 lumFactor * lum * cathodeXWeight );

    // int cathodeY = ( int( gl_FragCoord.y ) + 0 ) % 8;
    //int cathodeYWeight = ( ( int( gl_FragCoord.y ) + 1 ) % 8 > 0 ) ? 1 : 0;
    int cathodeYWeight = ( ( int( gl_FragCoord.y ) + 1 ) % 2 > 0 ) ? 1 : 1;
    color = mix( vec3( 0.0f, 0.0f, 0.0f ), color,
                 lumFactor * lum * cathodeYWeight );

    if ( abs( uv.x - 0.5f ) > 0.5f || abs( uv.y - 0.5f ) > 0.5f ) {
        color = vec3( 0.0f, 0.0f, 0.0f );
    }

    gFragColor = vec4( color, input.a );
}
