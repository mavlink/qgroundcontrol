#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static int set_pipe_flags(int fd, int flags)
{
    if ((flags & O_CLOEXEC) != 0) {
        const int fdFlags = fcntl(fd, F_GETFD);
        if ((fdFlags == -1) || (fcntl(fd, F_SETFD, fdFlags | FD_CLOEXEC) == -1)) {
            return -1;
        }
    }

    if ((flags & O_NONBLOCK) != 0) {
        const int statusFlags = fcntl(fd, F_GETFL);
        if ((statusFlags == -1) || (fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK) == -1)) {
            return -1;
        }
    }

    return 0;
}

// GStreamer 1.28.x's x86_64 iOS archive references pipe2, which the simulator SDK omits.
int pipe2(int fds[2], int flags)
{
    if ((flags & ~(O_CLOEXEC | O_NONBLOCK)) != 0) {
        errno = EINVAL;
        return -1;
    }

    if (pipe(fds) == -1) {
        return -1;
    }

    if ((set_pipe_flags(fds[0], flags) == -1) || (set_pipe_flags(fds[1], flags) == -1)) {
        const int savedErrno = errno;
        close(fds[0]);
        close(fds[1]);
        errno = savedErrno;
        return -1;
    }

    return 0;
}
