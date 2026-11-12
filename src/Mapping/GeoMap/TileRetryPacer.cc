#include "TileRetryPacer.h"

TileRetryPacer::TileRetryPacer(QObject* parent)
    : QObject(parent)
{
    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout, this, [this] {
        _intervalMs = qMin(_intervalMs * 2, kMaxIntervalMs);
        emit roundDue();
    });
}

void TileRetryPacer::setBaseIntervalMs(int baseIntervalMs)
{
    _baseIntervalMs = qMax(1, baseIntervalMs);
    _intervalMs = _baseIntervalMs;
}

void TileRetryPacer::tileFailed()
{
    if (!_timer.isActive()) {
        _timer.start(_intervalMs);
    }
}

void TileRetryPacer::tileArrived()
{
    _intervalMs = _baseIntervalMs;
}

void TileRetryPacer::reset()
{
    _timer.stop();
    _intervalMs = _baseIntervalMs;
}
