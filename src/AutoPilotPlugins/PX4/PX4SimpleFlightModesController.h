#pragma once

#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"
#include "QGCMAVLink.h"

/// MVC Controller for PX4SimpleFlightModes.qml
class PX4SimpleFlightModesController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
public:
    PX4SimpleFlightModesController(void);

    Q_PROPERTY(int          activeFlightMode    READ activeFlightMode       NOTIFY activeFlightModeChanged)
    Q_PROPERTY(int          channelCount        MEMBER _channelCount        CONSTANT)
    Q_PROPERTY(QVariantList rcChannelValues     MEMBER _rcChannelValues     NOTIFY rcChannelValuesChanged)

    int activeFlightMode(void) const { return _activeFlightMode; }

signals:
    void activeFlightModeChanged(int activeFlightMode);
    void channelOptionEnabledChanged(void);
    void rcChannelValuesChanged(void);

private slots:
    void channelValuesChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels]);

private:
    int             _activeFlightMode;
    int             _channelCount;
    QVariantList    _rcChannelValues;
};
