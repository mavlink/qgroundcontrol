#pragma once

#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

/// Controller for PX4FlightModes.qml. Supplies the live RC input and the names the vehicle reports for its external
/// modes. The mode slot and switch calculations are in PX4FlightModeRows.js.
class PX4FlightModesController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    /// true: rcChannelValues follows the vehicle's RC input. Off by default, for users that only need the mode names.
    Q_PROPERTY(bool monitorRc READ monitorRc WRITE setMonitorRc NOTIFY monitorRcChanged)
    /// Raw PWM of each RC channel the vehicle reports, while monitorRc is on. Empty when no RC_CHANNELS has arrived
    /// for a while.
    Q_PROPERTY(QVariantList rcChannelValues READ rcChannelValues NOTIFY rcChannelValuesChanged)
    /// Name the vehicle reports for each of External Mode 1-8, empty if it has not reported one
    Q_PROPERTY(QStringList externalModeNames READ externalModeNames NOTIFY externalModeNamesChanged)

public:
    explicit PX4FlightModesController(QObject* parent = nullptr);

    bool monitorRc() const { return _monitorRc; }

    void setMonitorRc(bool monitorRc);

    QVariantList rcChannelValues() const { return _rcChannelValues; }

    /// How long rcChannelValues are kept without a new RC_CHANNELS message
    void setRcTimeoutMs(int timeoutMs) { _rcTimeoutTimer.setInterval(timeoutMs); }

    QStringList externalModeNames() const { return _externalModeNames; }

signals:
    void monitorRcChanged();
    void rcChannelValuesChanged();
    void externalModeNamesChanged();

private slots:
    void _rcChannelsChanged(QVector<int> pwmValues);
    void _clearRcChannelValues();
    void _updateExternalModeNames();

private:
    bool _monitorRc = false;
    QVariantList _rcChannelValues;
    QTimer _rcTimeoutTimer;
    QStringList _externalModeNames;
};
