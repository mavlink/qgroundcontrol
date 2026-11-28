#include "ProcessFixtures.h"

#include <utility>

#include <QtCore/QSemaphore>
#include <QtCore/QThreadPool>
#include <QtCore/QtEnvironmentVariables>

#include "TestTimeout.h"

namespace TestFixtures {

// ============================================================================
// EnvVarFixture Implementation
// ============================================================================

EnvVarFixture::EnvVarFixture(const char* name)
    : _name(name)
    , _wasSet(qEnvironmentVariableIsSet(name))
{
    if (_wasSet) {
        _value = qgetenv(name);
    }
}

EnvVarFixture::~EnvVarFixture()
{
    if (!_name) {
        return;  // moved-from
    }
    if (_wasSet) {
        qputenv(_name, _value);
    } else {
        qunsetenv(_name);
    }
}

EnvVarFixture::EnvVarFixture(EnvVarFixture&& other) noexcept
    : _name(other._name)
    , _value(std::move(other._value))
    , _wasSet(other._wasSet)
{
    other._name = nullptr;
}

// ============================================================================
// ThreadPoolBlocker Implementation
// ============================================================================

ThreadPoolBlocker::ThreadPoolBlocker(QThreadPool* pool)
    : _letGo(std::make_shared<QSemaphore>())
{
    if (pool == nullptr) {
        pool = QThreadPool::globalInstance();
    }
    _threadCount = pool->maxThreadCount();
    const auto busy = std::make_shared<QSemaphore>();
    for (int thread = 0; thread < _threadCount; ++thread) {
        pool->start([busy, letGo = _letGo]() {
            busy->release();
            letGo->acquire();
        });
    }
    _blocking = busy->tryAcquire(_threadCount, TestTimeout::mediumMs());
}

ThreadPoolBlocker::~ThreadPoolBlocker()
{
    release();
}

void ThreadPoolBlocker::release()
{
    if (_letGo) {
        _letGo->release(_threadCount);
        _letGo.reset();
    }
}

}  // namespace TestFixtures
