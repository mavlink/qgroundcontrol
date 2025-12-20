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
    Q_PROPERTY(int          channelCount        MEMBER _channelCount        NOTIFY channelCountChanged)
    Q_PROPERTY(QVariantList rcChannelValues     MEMBER _rcChannelValues     NOTIFY rcChannelValuesChanged)

    int activeFlightMode(void) const { return _activeFlightMode; }

signals:
    void activeFlightModeChanged(int activeFlightMode);
    void channelOptionEnabledChanged(void);
    void rcChannelValuesChanged(void);
    void channelCountChanged();

private slots:
    void channelValuesChanged(QVector<int> pwmValues);

private:
    int             _activeFlightMode;
    int             _channelCount = 0;
    QVariantList    _rcChannelValues;
};
