#include "NTRIPConnectionStats.h"

#include <QtCore/QtMath>

NTRIPConnectionStats::NTRIPConnectionStats(QObject* parent)
    : QObject(parent)
{
    _rateTimer.setInterval(1000);
    connect(&_rateTimer, &QTimer::timeout, this, [this]() {
        const double rate = static_cast<double>(_bytesReceived - _dataRatePrevBytes);
        _dataRatePrevBytes = _bytesReceived;
        if (qAbs(_dataRateBytesPerSec - rate) > 1.0) {
            _dataRateBytesPerSec = rate;
            emit dataRateChanged();
        }
        emit bytesReceivedChanged();
        emit messagesReceivedChanged();
    });
}

void NTRIPConnectionStats::start()
{
    _rateTimer.start();
}

void NTRIPConnectionStats::stop()
{
    _rateTimer.stop();
    if (_dataRateBytesPerSec != 0.0) {
        _dataRateBytesPerSec = 0.0;
        _dataRatePrevBytes = 0;
        emit dataRateChanged();
    }
}

void NTRIPConnectionStats::recordMessage(int bytes)
{
    _bytesReceived += static_cast<quint64>(bytes);
    _messagesReceived++;
}

void NTRIPConnectionStats::reset()
{
    _bytesReceived = 0;
    _messagesReceived = 0;
    _dataRateBytesPerSec = 0.0;
    _dataRatePrevBytes = 0;
    emit bytesReceivedChanged();
    emit messagesReceivedChanged();
    emit dataRateChanged();
}
