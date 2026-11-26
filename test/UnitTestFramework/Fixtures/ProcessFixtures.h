#pragma once

#include <memory>

#include <QtCore/QByteArray>

class QSemaphore;

/// @file
/// @brief RAII guards for state that the whole test process shares: environment variables and the global thread
/// pool. They need nothing of the application, so the portable test executables use them too.

namespace TestFixtures {

// ============================================================================
// EnvVarFixture - RAII guard for environment variable save/restore
// ============================================================================

/// RAII guard that saves an environment variable on construction and restores it on destruction.
class EnvVarFixture
{
public:
    explicit EnvVarFixture(const char* name);
    ~EnvVarFixture();

    EnvVarFixture(EnvVarFixture&& other) noexcept;

    EnvVarFixture(const EnvVarFixture&) = delete;
    EnvVarFixture& operator=(const EnvVarFixture&) = delete;
    EnvVarFixture& operator=(EnvVarFixture&&) = delete;

private:
    const char* _name;
    QByteArray _value;
    bool _wasSet;
};

// ============================================================================
// ThreadPoolBlocker - RAII wrapper that holds back work on the global thread pool
// ============================================================================

/// RAII fixture that occupies every thread of QThreadPool::globalInstance() until release() or destruction. Work
/// started meanwhile, such as the worker of a CompressionJob, waits for a thread: it is held back without a
/// timing assumption.
class ThreadPoolBlocker
{
public:
    /// Returns once every thread is occupied, or after TestTimeout::mediumMs(): check isBlocking()
    ThreadPoolBlocker();
    ~ThreadPoolBlocker();

    ThreadPoolBlocker(const ThreadPoolBlocker&) = delete;
    ThreadPoolBlocker& operator=(const ThreadPoolBlocker&) = delete;

    /// Whether every thread of the pool was occupied
    bool isBlocking() const { return _blocking; }

    /// Let the threads go
    void release();

private:
    std::shared_ptr<QSemaphore> _letGo;
    int _threadCount = 0;
    bool _blocking = false;
};

}  // namespace TestFixtures
