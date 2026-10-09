#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#ifdef Q_OS_LINUX
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <thread>
#include <unistd.h>

namespace GPSTest {

/// Linux pseudo-terminal for serial tests. The code under test opens slavePath() as its serial device and the test
/// acts as the device through the non-blocking master.
class PseudoTerminal
{
public:
    PseudoTerminal()
        : _master(::posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK))
    {
        if (_master < 0 || ::grantpt(_master) != 0 || ::unlockpt(_master) != 0) {
            return;
        }
        const char* const slave = ::ptsname(_master);
        if (!slave) {
            return;
        }
        // Opened ahead of the device under test: a serial port's exclusive open refuses later openers.
        _outputProbe = ::open(slave, O_WRONLY | O_NOCTTY | O_CLOEXEC | O_NONBLOCK);
        if (_outputProbe >= 0) {
            _slavePath = QString::fromLocal8Bit(slave);
        }
    }

    ~PseudoTerminal()
    {
        closeMaster();
        if (_outputProbe >= 0) {
            ::close(_outputProbe);
        }
    }

    PseudoTerminal(const PseudoTerminal&) = delete;
    PseudoTerminal& operator=(const PseudoTerminal&) = delete;

    bool isValid() const { return !_slavePath.isEmpty(); }

    const QString& slavePath() const { return _slavePath; }

    int masterHandle() const { return _master; }

    /// Hangs up the terminal, as when the device is unplugged.
    void closeMaster()
    {
        if (_master >= 0) {
            ::close(_master);
            _master = -1;
        }
    }

    /// The bytes the device under test has written so far, without waiting.
    QByteArray readAvailable()
    {
        QByteArray received;
        char buffer[4096];
        for (;;) {
            const auto count = ::read(_master, buffer, sizeof(buffer));
            if (count <= 0) {
                return received;
            }
            received.append(buffer, count);
        }
    }

    /// Waits until the unread output fills the terminal for good, so a writer on the slave side has to block. Just
    /// after the terminal first fills, the line discipline moves queued output into its read buffer and frees room
    /// again, so the terminal counts as full once it has stayed full for STABLE_BLOCK.
    bool waitForBlockedOutput(std::chrono::milliseconds timeout) const
    {
        if (_outputProbe < 0) {
            return false;
        }
        const QDeadlineTimer deadline(timeout);
        QDeadlineTimer stable(STABLE_BLOCK);
        pollfd output{.fd = _outputProbe, .events = POLLOUT, .revents = 0};
        while (!stable.hasExpired()) {
            if (::poll(&output, 1, 0) != 0) {
                stable.setRemainingTime(STABLE_BLOCK);
            }
            if (deadline.hasExpired()) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    static constexpr std::chrono::milliseconds STABLE_BLOCK{10};

private:
    int _master = -1;
    int _outputProbe = -1;
    QString _slavePath;
};

}  // namespace GPSTest

#endif
