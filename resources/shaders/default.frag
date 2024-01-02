#version 330 core

in vec4 gColor;
in vec3 gNormal;
in vec3 gPos;
in vec2 gTextureCoords;

out vec4 gFragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;
uniform vec3 uEyePos;
// uniform float uTintIntensity;

uniform sampler2D uLightTexture;
uniform mat4 uLightTransform; // world to clip
uniform vec3 uLightDirection;

const highp float kDither = 0.5 / 255.0;
const float G = 0.0000001f; // scatter constant
const float PI = 3.14159265f;
const int kVolumetricSteps = 100;
const mat4 DITHER_PATTERN = mat4( vec4( 0.0f, 0.5f, 0.125f, 0.625f ),
                                  vec4( 0.75f, 0.22f, 0.875f, 0.375f ),
                                  vec4( 0.1875f, 0.6875f, 0.0625f, 0.5625f ),
                                  vec4( 0.9375f, 0.4375f, 0.8125f, 0.3125f ) );

struct SpotLight {
    vec3 pos;
    vec3 dir;
    float fovCosine;
};

SpotLight gSpotLight =
    SpotLight( vec3( 1.0f, 1.0f, 3.0f ), vec3( -1.0f, 0.0f, -3.0f ), 0.8f );

float spotLightFactor( vec3 samplePos ) {
    vec4 lightCoord = uLightTransform * vec4( samplePos, 1.0f );
    lightCoord = lightCoord / lightCoord.w;

    return ( abs( lightCoord.x ) < 1.0f && abs( lightCoord.y ) < 1.0f &&
             abs( lightCoord.z ) < 1.0f )
               ? ( 1.0f - max( 0.0f, lightCoord.z ) )
               : 0.0f;
}

vec3 spotLightColor( vec3 samplePos ) {
    vec4 lightCoord = uLightTransform * vec4( samplePos, 1.0f );
    lightCoord = lightCoord / lightCoord.w;

    float factor = ( abs( lightCoord.x ) < 1.0f && abs( lightCoord.y ) < 1.0f &&
                     abs( lightCoord.z ) < 1.0f )
                       ? ( 1.0f - max( 0.0f, lightCoord.z ) )
                       : 0.0f;

    vec2 texCoords =
        vec2( -lightCoord.x * 0.5f + 0.5f, -lightCoord.y * 0.5f + 0.5f );

    vec4 color = texture( uLightTexture, texCoords );

    return color.rgb * factor;
}

float scattering( float cosTheta ) {
    return ( ( 1.0f - G * G ) ) /
           ( 4.0f * PI * pow( 1.0f + G * G - 2.0f * G * cosTheta, 1.5f ) );
}

highp float randomX( vec2 coords ) {
    return fract( sin( dot( coords.xy, vec2( 12.9898f, 78.233f ) ) ) *
                  43758.5453f );
}

highp float randomY( vec2 coords ) {
    return fract( sin( dot( coords.xy, vec2( 92.9898f, 7.803f ) ) ) *
                  93258.5453f );
}

highp float randomZ( vec2 coords ) {
    return fract( sin( dot( coords.xy, vec2( 2.0898f, 49.134f ) ) ) *
                  21381.1252f );
}

vec3 volumetricSample() {
    vec3 stepDir = normalize( uEyePos - gPos );
    vec3 step = ( uEyePos - gPos ) / ( kVolumetricSteps * 1.0f );

    vec3 total = vec3( 0.0f, 0.0f, 0.0f );

    int screenX = int( gl_FragCoord.x );
    int screenY = int( gl_FragCoord.y );

    vec3 startPos = gPos + step * DITHER_PATTERN[ screenX % 4 ][ screenY % 4 ];
    // vec3 startPos = gPos + step * randomX(gl_FragCoord.xy);

    for ( int i = 0; i < kVolumetricSteps; i++ ) {
        vec3 samplePos = startPos + i * step;
        float cosTheta = dot( uLightDirection, stepDir );

        // if ( spotLightFactor( samplePos ) > 0.5f ) {
        //     cosTheta = dot( normalize( samplePos ), stepDir );
        // }

        // cosTheta = 1.0f + cosTheta * 0.0001f ;
        float factor2 = clamp( cosTheta + 0.6f, 0.0f, 1.0f );
        factor2 = 1.0f;
        total = total +
                factor2 * scattering( cosTheta ) * spotLightColor( samplePos );
    }

    return total / kVolumetricSteps;
}

vec4 textureColor() {
    return uUseTexture * texture( uTexture, gTextureCoords ) +
           ( 1 - uUseTexture ) * vec4( 1.0f, 1.0f, 1.0f, 1.0f );
}

void main() {
    vec4 baseColor = gColor * textureColor();
    // gFragColor = mix(baseColor, gColor, uTintIntensity) ;

    // const vec3 lightDirection = vec3( 1.0, 2.0, 3.0 );

    float light =
        max( 0.0f, 0.11f * dot( -gNormal, normalize( gSpotLight.dir ) ) ) +
        max( 0.0f, 0.4f * spotLightFactor( gPos ) *
                       dot( -gNormal, uLightDirection ) );
    // float light = 1.0;
    if ( uUseTexture == 0 ) {
        light = clamp( light, 0.0f, 1.0f ) + 0.1f;
    } else {
        light = 1.0f;
    }

    // only want light to affect the color, not the alpha
    vec3 lightedColor = light * baseColor.rgb + volumetricSample() * 10.0f;

    int screenX = int( gl_FragCoord.x );
    int screenY = int( gl_FragCoord.y );

    const float ditherFactor = 0.1f;

    gFragColor = vec4( lightedColor, baseColor.a );

    gFragColor.r += mix( -kDither, kDither, randomX( gl_FragCoord.xy ) );
    gFragColor.g += mix( -kDither, kDither, randomY( gl_FragCoord.xy ) );
    gFragColor.b += mix( -kDither, kDither, randomZ( gl_FragCoord.xy ) );

    // gFragColor.r +=
    //     DITHER_PATTERN[ ( screenX + 0 ) % 4 ][ ( screenY + 4 ) % 4 ] *
    //     ditherFactor;
    // gFragColor.g +=
    //     DITHER_PATTERN[ ( screenX + 1 ) % 4 ][ ( screenY + 1 ) % 4 ] *
    //     ditherFactor;
    // gFragColor.b +=
    //     DITHER_PATTERN[ ( screenX + 3 ) % 4 ][ ( screenY + 0 ) % 4 ] *
    //     ditherFactor;
}
