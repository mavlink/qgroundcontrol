#include "RCChannelMonitorController.h"
#include "Vehicle.h"
#include <QtCore/QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(RCChannelMonitorControllerLog, "QMLControls.RCChannelMonitorController")

RCChannelMonitorController::RCChannelMonitorController(QObject *parent)
    : FactPanelController(parent)
{
    // qCDebug(RCChannelMonitorControllerLog) << Q_FUNC_INFO << this;

    (void) connect(_vehicle, &Vehicle::rcChannelsChanged, this, &RCChannelMonitorController::channelValuesChanged);
}

RCChannelMonitorController::~RCChannelMonitorController()
{
    // qCDebug(RCChannelMonitorControllerLog) << Q_FUNC_INFO << this;
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
