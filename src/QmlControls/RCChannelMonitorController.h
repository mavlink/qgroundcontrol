/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "FactPanelController.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(RCChannelMonitorControllerLog)

class RCChannelMonitorController : public FactPanelController
{
    Q_OBJECT
    // QML_ELEMENT
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelCountChanged)

public:
    explicit RCChannelMonitorController(QObject *parent = nullptr);
    ~RCChannelMonitorController();

    int channelCount() const { return _chanCount; }

signals:
    void channelCountChanged(int channelCount);
    void channelRCValueChanged(int channel, int rcValue);

private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels]);

private:
    int _chanCount = 0;
};
