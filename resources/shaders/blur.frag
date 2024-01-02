#version 330 core

in vec2 gTextureCoords;

out vec4 gFragColor;

uniform sampler2D uTexture;
uniform vec2 uBlurStep;

// const float kKernel[ 9 ] = float[]( +0.0625f, +0.1250f, +0.0625f, //
//                                     +0.1250f, +0.2500f, +0.1250f, //
//                                     +0.0625f, +0.1250f, +0.0625f  //
//);

// const float kGaussian[ 9 ] =
//     float[]( 0.016216, 0.054054, 0.1216216, 0.1945946, 0.227027, 0.1945946,
//              0.1216216, 0.054054, 0.016216 );

 const float kGaussian[ 9 ] =
     float[]( 0.016216, 0.054054, 0.1216216, 0.1945946, 1.000000, 0.1945946,
              0.1216216, 0.054054, 0.016216 );

// const float kGaussian[ 9 ] =
//     float[]( 0.1, 0.1, 0.1, 0.1, 1.000000, 0.1,
//              0.1, 0.1, 0.1 );

// const float kId[ 9 ] = float[]( 0.01, 0.1, 0.12, 0.2001, 1.0, 0.2001, 0.12,
// 0.1, 0.01 );

// void main() {
//     vec2 centerCoord = gTextureCoords;
//     vec4 totalColor = texture( uTexture, centerCoord );
//
//     int sampleRange = 100;
//     int sampleHRange = 50;
//     int sampleCount = 10;
//     float weight = 1.00f;
//
//     // some random sampling
//     for ( int i = 0; i < sampleCount; i++ ) {
//         float rand1 = fract( sin( 8.21308 * gl_FragCoord.x +
//                                   0.90984 * gl_FragCoord.y + 9.0120984 * i )
//                                   *
//                              2.02138 );
//
//         float rand2 = fract( cos( 1.45793 * gl_FragCoord.x +
//                                   0.37083 * gl_FragCoord.y + 4.3580243 * i )
//                                   *
//                              2.02138 );
//
//         int sampleX = int( rand1 * sampleRange ) - sampleHRange;
//         int sampleY = int( rand2 * sampleRange ) - sampleHRange;
//
//         float tx = uBlurStep.x * sampleX;
//         float ty = uBlurStep.y * sampleY;
//         vec2 dt = vec2( sampleX, sampleY );
//         vec2 dt2 = vec2( tx, ty );
//         vec2 sampleCoord = centerCoord + dt2;
//         float decay = dot( dt, dt );
//         float w = weight * min( 1.0f, 100.0f / (  decay ));
//         // vec2 sampleCoord = gTextureCoords + 0.0001f * vec2( sx, sy );
//
//         totalColor += w * texture( uTexture, sampleCoord );
//     }
//
//     gFragColor = totalColor;
// }

void main() {

    const int kernSize = 9;
    const int hkernSize = kernSize / 2;

    vec2 uv = gTextureCoords;

    // vec2 dc = abs( 0.5f - uv );
    // dc *= dc;

    // const float warp = 0.75f;

    //// warp the fragment coordinates
    // uv.x -= 0.5f;
    // uv.x *= 1.0f + ( dc.y * ( 0.3f * warp ) );
    // uv.x += 0.5f;
    // uv.y -= 0.5f;
    // uv.y *= 1.0f + ( dc.x * ( 0.4f * warp ) );
    // uv.y += 0.5f;

    vec4 totalColor = vec4( 0.0f, 0.0f, 0.0f, 0.0f );
    for ( int i = 0; i < kernSize; i++ ) {
        for ( int j = 0; j < kernSize; j++ ) {
            float sx = 1.0f * uBlurStep.x * ( i - hkernSize );
            float sy = 1.0f * uBlurStep.y * ( j - hkernSize );

            // float weight = kKernel[ i + j * kernSize ];
            float weight = kGaussian[ i ] * kGaussian[ j ];
            //  float weight = kId[ i ] * kId[ j ];

            vec2 sampleCoord = uv + vec2( sx, sy );
            // vec2 sampleCoord = gTextureCoords + 0.0001f * vec2( sx, sy );

            totalColor += (weight + 0.03) * texture( uTexture, sampleCoord );
        }
    }

    //totalColor = mix(texture(uTexture, uv), totalColor, 0.0001f);

    // if ( abs( uv.x - 0.5f ) > 0.5f || abs( uv.y - 0.5f ) > 0.5f ) {
    //     totalColor = vec4( 0.0f, 0.0f, 0.0f, 1.0f );
    // }

    gFragColor = totalColor;
}
