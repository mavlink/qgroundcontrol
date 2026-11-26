#include "VideoBackendInit.h"

#include <QtConcurrent/QtConcurrentRun>

#include "VideoBackend.h"

VideoBackendInit::VideoBackendInit(QObject* parent)
    : QObject(parent)
{}

void VideoBackendInit::start()
{
    if (_state != State::NotStarted) {
        return;
    }

    VideoBackend& backend = VideoBackend::instance();
    if (!backend.capabilities().asyncInit || VideoBackend::disabledForUnitTests()) {
        _state = State::Ready;
        return;
    }

    _state = State::Pending;
    const VideoBackend::InitParams params{.environment = backend.prepareEnvironment(),
                                          .debugLevel = VideoBackend::persistedDebugLevel()};
    start(QtConcurrent::run([&backend, params] { return backend.initialize(params); }));
}

void VideoBackendInit::start(const QFuture<bool>& initialization)
{
    _state = State::Pending;
    _initialization = initialization;
    // Ignored when waitUntilFinished() already took the result.
    (void) _initialization.then(this, [this](bool ready) { _finish(ready); }).onCanceled(this, [this] {
        _finish(false);
    });
}

bool VideoBackendInit::waitUntilFinished()
{
    start();
    if (_state == State::Pending) {
        _initialization.waitForFinished();
        _finish(!_initialization.isCanceled() && (_initialization.resultCount() > 0) && _initialization.result());
    }
    return _state == State::Ready;
}

void VideoBackendInit::_finish(bool ready)
{
    if (_state != State::Pending) {
        return;
    }
    _state = ready ? State::Ready : State::Failed;
    emit finished(ready);
}
