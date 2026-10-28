#pragma once

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

/// Controller for PX4FlightModes.qml. Supplies the live RC input and the names the vehicle reports for its external
/// modes. The mode slot and switch calculations are in PX4FlightModeRows.js, the mode names and mode selection rules in
/// PX4FlightModeConfig.
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

    /// "switch", "buttons" or "none" from RC_MAP_FLTMODE and RC_MAP_FLTM_BTN
    Q_INVOKABLE QString selectionMethod(int modeChannel, int buttonMask) const;

    /// RC channels (1-based) of the mode buttons in RC_MAP_FLTM_BTN, lowest first. Button N selects slot N.
    Q_INVOKABLE QVariantList buttonChannels(int buttonMask) const;

    /// Channels set in RC_MAP_FLTM_BTN that PX4 ignores, because only the lowest six get a slot
    Q_INVOKABLE QVariantList ignoredButtonChannels(int buttonMask) const;

    /// [{ label, value }] for a mode parameter. An external mode is offered under the name the vehicle reports for it,
    /// and left out when nothing is registered there, unless currentValue already selects it. The arguments are passed
    /// rather than read here so a QML binding on them updates.
    Q_INVOKABLE QVariantList modeChoices(const QString& paramName, const QStringList& enumStrings,
                                         const QVariantList& enumValues, const QStringList& externalNames,
                                         int currentValue) const;

    /// Label of the mode a parameter value selects, as modeChoices shows it. Empty for a value with no choice.
    Q_INVOKABLE QString modeName(const QString& paramName, const QStringList& enumStrings,
                                 const QVariantList& enumValues, const QStringList& externalNames, int value) const;

signals:
    void monitorRcChanged();
    void rcChannelValuesChanged();
    void externalModeNamesChanged();

private slots:
    void _rcChannelsChanged(QVector<int> pwmValues);
    void _clearRcChannelValues();
    void _updateExternalModeNames();

private:
    static QVariantList _toVariantList(const QList<int>& values);

    bool _monitorRc = false;
    QVariantList _rcChannelValues;
    QTimer _rcTimeoutTimer;
    QStringList _externalModeNames;
};
