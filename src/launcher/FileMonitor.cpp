#include "FileMonitor.h"

#include "Logging.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
//#include <linux/inotify.h>

#include <sstream>
#include <stdexcept>

#define ASSERT( expr )                                                         \
    if ( !( expr ) ) {                                                         \
        std::stringstream ss;                                                  \
        ss << "ASSERT FAILED: " << #expr << " at " << __FILE__ << ":"          \
           << __LINE__;                                                        \
        throw std::runtime_error( ss.str() );                                  \
    }

namespace fileMonitor {

static bool readEvents( Info & info ) {
    /* Some systems cannot read integer variables if they are not
       properly aligned. On other systems, incorrect alignment may
       decrease performance. Hence, the buffer used for reading from
       the inotify file descriptor should have the same alignment as
       struct inotify_event. */

    char buf[ 4096 ]
        __attribute__( ( aligned( __alignof__( struct inotify_event ) ) ) );

    const int fd = info.notifyFd;
    const struct inotify_event * event;
    ssize_t len;

    /* Loop while events can be read from inotify file descriptor. */
    bool updated = false;

    for ( ;; ) {

        /* Read some events. */

        len = read( fd, buf, sizeof( buf ) );
        ASSERT( !( len == -1 && errno != EAGAIN ) );

        /* If the nonblocking read() found no events to read, then
           it returns -1 with errno set to EAGAIN. In that case,
           we exit the loop. */

        if ( len <= 0 )
            break;

        // printf( "bytes read: %zd \n", len );

        /* Loop over all events in the buffer. */

        for ( char * ptr = buf; ptr < buf + len;
              ptr += sizeof( struct inotify_event ) + event->len ) {

            event = (const struct inotify_event *) ptr;

            // printf( "event mask: %x\n", event->mask );

            if ( !event->len )
                continue;
            std::string filename( event->name );

            // DEBUG_LOG() << "ANON FILE EVENT OCCURED: " << filename <<
            // std::endl;

            if ( !info.wholeDirectory &&
                 filename != info.path.filename().string() )
                continue;

            // DEBUG_LOG() << "DESIRED FILE EVENT OCCURED: " << filename <<
            // std::endl;

            if ( event->mask & IN_OPEN )
                DEBUG_LOG() << "IN_OPEN" << std::endl;
            if ( event->mask & IN_CLOSE_NOWRITE )
                DEBUG_LOG() << "IN_CLOSE_NOWRITE" << std::endl;

            if ( event->mask & IN_CLOSE_WRITE ) {
                DEBUG_LOG() << "IN_CLOSE_WRITE" << std::endl;
                updated = true;
            }

            if ( event->mask & IN_MODIFY )
                DEBUG_LOG() << "IN_MODIFY" << std::endl;
            if ( event->mask & IN_ACCESS )
                DEBUG_LOG() << "IN_ACCESS" << std::endl;
            if ( event->mask & IN_ATTRIB )
                DEBUG_LOG() << "IN_ATTRIB" << std::endl;
            if ( event->mask & IN_CREATE )
                DEBUG_LOG() << "IN_CREATE" << std::endl;
            if ( event->mask & IN_DELETE )
                DEBUG_LOG() << "IN_DELETE" << std::endl;
            if ( event->mask & IN_IGNORED )
                DEBUG_LOG() << "IN_IGNORE" << std::endl;
        }
    }

    return updated;
}

void start( Info & info ) {
    const int fd = inotify_init1( IN_NONBLOCK );
    ASSERT( fd != -1 );
    const uint32_t flags = IN_OPEN | IN_CLOSE | IN_CREATE | IN_DELETE;

    std::string parentPath = info.path.parent_path().string();
    if ( info.wholeDirectory ) {
        parentPath = info.path.string();
    }

    const int wd = inotify_add_watch( fd, parentPath.c_str(), flags );
    ASSERT( wd != -1 );

    info.notifyFd = fd;
}

bool fileUpdated( Info & info ) {
    pollfd pfd;

    pfd.fd = info.notifyFd;
    pfd.events = POLLIN;

    const int n = poll( &pfd, 1, 0 );

    ASSERT( n != -1 );

    if ( n > 0 ) {
        if ( pfd.revents & POLLIN ) {
            // read events
            return readEvents( info );
        }
    }

    return false;
}

void stop( Info & info ) {
    close( info.notifyFd );
}

} // namespace fileMonitor
