#include "RCChannelMonitorController.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RCChannelMonitorControllerLog, "QMLControls.RCChannelMonitorController")

RCChannelMonitorController::RCChannelMonitorController(QObject *parent)
    : FactPanelController(parent)
{
    // qCDebug(RCChannelMonitorControllerLog) << Q_FUNC_INFO << this;

    _connectToSignal();
}

RCChannelMonitorController::~RCChannelMonitorController()
{
    // qCDebug(RCChannelMonitorControllerLog) << Q_FUNC_INFO << this;
}

void RCChannelMonitorController::setClampValues(bool clamp)
{
    if (_clampValues != clamp) {
        _clampValues = clamp;
        _connectToSignal();
        emit clampValuesChanged();
    }
}

void RCChannelMonitorController::_connectToSignal()
{
    disconnect(_vehicle, &Vehicle::rcChannelsClampedChanged, this, &RCChannelMonitorController::channelValuesChanged);
    disconnect(_vehicle, &Vehicle::rcChannelsRawChanged, this, &RCChannelMonitorController::channelValuesChanged);
    if (_clampValues) {
        (void) connect(_vehicle, &Vehicle::rcChannelsClampedChanged, this, &RCChannelMonitorController::channelValuesChanged);
    } else {
        (void) connect(_vehicle, &Vehicle::rcChannelsRawChanged, this, &RCChannelMonitorController::channelValuesChanged);
    }
}

void RCChannelMonitorController::channelValuesChanged(QVector<int> pwmValues)
{
    int channelCount = pwmValues.size();

    if (_chanCount != channelCount) {
        _chanCount = channelCount;
        emit channelCountChanged(_chanCount);
    }

    for (int channel = 0; channel < channelCount; channel++) {
        emit channelValueChanged(channel, pwmValues[channel]);
    }
}
