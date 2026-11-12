#pragma once

// The checks of the pipe to a link another thread serves: GPSPipe, GPSPipeTransport and GPSPipeLink. They need no
// unit test framework: GPSPipeTransportTest runs them in the application, and QGCGPSPipeTransportChecks
// (../Checks) without it, for test/GPS/Standalone to run them under its compiler and sanitizers.

#include <span>

#include <QtCore/QString>

namespace GPSTest {

struct PipeTransportCheck
{
    /// "threads-" checks run the session on a thread of its own against a link that acts from another, with waits
    /// that no deadline ends. "clock-" checks run on a virtual clock, on one thread. "link-" checks run a GPSPipeLink
    /// on the calling thread's event loop, which needs a QCoreApplication.
    const char* name;
    /// Returns nothing when the check passes, else where it failed.
    QString (*run)();
};

std::span<const PipeTransportCheck> pipeTransportChecks();

}  // namespace GPSTest
