#pragma once

#include <cmath>
#include <filesystem>
#include <vector>

// class Map;

namespace utils {

// class MapRaii {
//   public:
//     MapRaii( char * data );
//     ~MapRaii();
//
//     Map * operator->() const {
//         return map;
//     }
//
//     Map & operator*() const {
//         return *map;
//     }
//
//     Map * get() const {
//         return map;
//     }
//
//   private:
//     Map * map;
// };

class Noncopyable {
  public:
    Noncopyable() = default;
    Noncopyable( const Noncopyable & ) = delete;
    Noncopyable & operator=( const Noncopyable & ) = delete;
};

class ManualTimer {
  public:
    using Callback = void ( * )();

    ManualTimer() : ManualTimer( 1.0f ) {}

    ManualTimer( float duration, Callback callback = nullptr,
                 bool looping = false )
        : mDuration( duration ), mCallback( callback ), mLooping( looping ) {
        if ( mDuration == 0.0f ) {
            throw std::runtime_error(
                "Timer cannot have 0.0 second duration." );
        }
        start();
    }

    void start() {
        mElapsed = 0.0f;
    }

    void tick( float timestep ) {
        bool wasFinished = isFinished();

        if ( wasFinished && !mLooping )
            return;

        mElapsed += timestep;

        if ( isFinished() ) {
            if ( !wasFinished && mCallback ) {
                mCallback();
            }
            if ( mLooping ) {
                mElapsed = std::fmod( mElapsed, mDuration );
            } else {
                // mElapsed = mDuration;
            }
        }
    }

    bool isFinished() {
        return mElapsed >= mDuration;
    }

    float progress() {
        return std::min( 1.0f, mElapsed / mDuration );
    }

    void setDuration( float dur ) {
        mDuration = dur;
    }

    void setLooping( bool enable ) {
        mLooping = enable;
    }

  private:
    float mDuration;
    float mElapsed;
    Callback mCallback;
    bool mLooping;
};

template < typename T >
void appendVector( std::vector< T > & dest, const std::vector< T > & tail ) {
    dest.insert( dest.end(), tail.begin(), tail.end() );
}

std::string stringFromFile( std::filesystem::path path );

} // namespace utils
