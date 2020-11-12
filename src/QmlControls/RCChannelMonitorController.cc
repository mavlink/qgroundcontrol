/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "RCChannelMonitorController.h"

RCChannelMonitorController::RCChannelMonitorController(void)
    : _chanCount(0)
{
    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &RCChannelMonitorController::_rcChannelsChanged);
}

void RCChannelMonitorController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    for (int channel=0; channel<channelCount; channel++) {
        int channelValue = pwmValues[channel];

        if (_chanCount != channelCount) {
            _chanCount = channelCount;
            emit channelCountChanged(_chanCount);
        }

        if (channelValue != -1) {
            emit channelRCValueChanged(channel, channelValue);
        }
    }
}
