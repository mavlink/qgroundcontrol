/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "FactPanelController.h"
#include "QGCMAVLink.h"

class RCChannelMonitorController : public FactPanelController
{
    Q_OBJECT

public:
    RCChannelMonitorController(void);

    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelCountChanged)

    int channelCount(void) const{ return _chanCount; }

signals:
    void channelCountChanged(int channelCount);
    void channelRCValueChanged(int channel, int rcValue);

private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels]);

private:
    int _chanCount;
};
