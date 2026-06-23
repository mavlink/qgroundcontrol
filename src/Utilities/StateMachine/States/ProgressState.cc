#include "ProgressState.h"

#include <algorithm>

ProgressState::ProgressState(const QString& stateName, QState* parent, float progress, Action action)
    : QGCState(stateName, parent)
    , _fixedProgress(std::clamp(progress, 0.0f, 1.0f))
    , _action(std::move(action))
    , _useCallback(false)
{
}

ProgressState::ProgressState(const QString& stateName, QState* parent, ProgressCallback progressCallback, Action action)
    : QGCState(stateName, parent)
    , _progressCallback(std::move(progressCallback))
    , _action(std::move(action))
    , _useCallback(true)
{
}

float ProgressState::progress() const
{
    if (_useCallback && _progressCallback) {
        return std::clamp(_progressCallback(), 0.0f, 1.0f);
    }
    return _fixedProgress;
}

void ProgressState::setProgress(float progress)
{
    _fixedProgress = std::clamp(progress, 0.0f, 1.0f);
    _useCallback = false;
}

void ProgressState::setProgressCallback(ProgressCallback callback)
{
    _progressCallback = std::move(callback);
    _useCallback = true;
}

void ProgressState::onEnter()
{
    emit progressChanged(progress());

    if (_action) {
        _action();
    }

    emit advance();
}
