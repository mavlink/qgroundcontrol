#include "RCChannelMonitorController.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RCChannelMonitorControllerLog, "QMLControls.RCChannelMonitorController")

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

void RCChannelMonitorController::channelValuesChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels])
{
    for (int channel = 0; channel < channelCount; channel++) {
        const int channelValue = pwmValues[channel];

        if (_chanCount != channelCount) {
            _chanCount = channelCount;
            emit channelCountChanged(_chanCount);
        }

        if (channelValue != -1) {
            emit channelValueChanged(channel, channelValue);
        }
    }
}
