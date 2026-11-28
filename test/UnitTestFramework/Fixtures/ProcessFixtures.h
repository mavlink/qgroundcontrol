#pragma once

#include <memory>

#include <QtCore/QByteArray>

class QSemaphore;
class QThreadPool;

/// @file
/// @brief RAII guards for state that the whole test process shares: environment variables and the thread
/// pools. They need nothing of the application, so the portable test executables use them too.

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
// ThreadPoolBlocker - RAII wrapper that holds back work on a thread pool
// ============================================================================

/// RAII fixture that occupies every thread of a thread pool until release() or destruction. Work started on it
/// meanwhile, such as the worker of a QGC::runOnWorker() job on the global pool, or of a CompressionJob on
/// CompressionJob::threadPool(), waits for a thread: it is held back without a timing assumption.
class ThreadPoolBlocker
{
public:
    /// Returns once every thread is occupied, or after TestTimeout::mediumMs(): check isBlocking()
    /// @param pool The pool to occupy, QThreadPool::globalInstance() if null
    explicit ThreadPoolBlocker(QThreadPool* pool = nullptr);
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
