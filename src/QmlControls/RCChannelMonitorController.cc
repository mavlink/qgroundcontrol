/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "RCChannelMonitorController.h"

RCChannelMonitorController::RCChannelMonitorController(void)
    : _chanCount(0)
{
    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &RCChannelMonitorController::_rcChannelsChanged);
}

void RCChannelMonitorController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    int maxChannel = std::min(channelCount, _chanMax());

    for (int channel=0; channel<maxChannel; channel++) {
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

int RCChannelMonitorController::_chanMax(void) const
{
    return _vehicle->firmwareType() == MAV_AUTOPILOT_PX4 ? _chanMaxPX4 : _chanMaxAPM;
}
