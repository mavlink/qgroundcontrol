/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QStringList>
#include <QtCore/QVariantList>

#include "FactPanelController.h"
#include "QGCMAVLink.h"

/// MVC Controller for FlightModesComponent.qml.
class APMFlightModesComponentController : public FactPanelController
{
    Q_OBJECT
    Q_PROPERTY(QString      modeParamPrefix         MEMBER _modeParamPrefix         CONSTANT)
    Q_PROPERTY(QString      modeChannelParam        MEMBER _modeChannelParam        CONSTANT)
    Q_PROPERTY(int          activeFlightMode        READ activeFlightMode           NOTIFY activeFlightModeChanged)
    Q_PROPERTY(int          channelCount            MEMBER _channelCount            CONSTANT)
    Q_PROPERTY(QVariantList channelOptionEnabled    READ channelOptionEnabled       NOTIFY channelOptionEnabledChanged)
    Q_PROPERTY(bool         simpleModesSupported    MEMBER _simpleModesSupported    CONSTANT)
    Q_PROPERTY(QStringList  simpleModeNames         MEMBER _simpleModeNames         CONSTANT)
    Q_PROPERTY(int          simpleMode              MEMBER _simpleMode              NOTIFY simpleModeChanged)
    Q_PROPERTY(QVariantList simpleModeEnabled       MEMBER _simpleModeEnabled       NOTIFY simpleModeEnabledChanged)
    Q_PROPERTY(QVariantList superSimpleModeEnabled  MEMBER _superSimpleModeEnabled  NOTIFY superSimpleModeEnabledChanged)

public:
    explicit APMFlightModesComponentController(QObject *parent = nullptr);

    enum SimpleModeValues {
        SimpleModeStandard = 0,
        SimpleModeSimple,
        SimpleModeSuperSimple,
        SimpleModeCustom
    };
    Q_ENUM(SimpleModeValues)

    Q_INVOKABLE void setSimpleMode(int fltModeIndex, bool enabled);
    Q_INVOKABLE void setSuperSimpleMode(int fltModeIndex, bool enabled);

    int activeFlightMode() const { return _activeFlightMode; }
    QVariantList channelOptionEnabled() const { return _rgChannelOptionEnabled; }

signals:
    void activeFlightModeChanged(int activeFlightMode);
    void channelOptionEnabledChanged();
    void simpleModeChanged(int simpleMode);
    void simpleModeEnabledChanged();
    void superSimpleModeEnabledChanged();

private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels]);
    void _updateSimpleParamsFromSimpleMode();
    void _setupSimpleModeEnabled();

private:
    Fact *_simpleModeFact = nullptr;
    Fact *_superSimpleModeFact = nullptr;
    const bool _simpleModesSupported = false;
    int _activeFlightMode = 0;
    int _channelCount = QGCMAVLink::maxRcChannels;
    int _simpleMode = SimpleModeStandard;
    QString _modeChannelParam;
    QString _modeParamPrefix;
    const QStringList _simpleModeNames = { tr("Off"), tr("Simple"), tr("Super-Simple"), tr("Custom") };
    QVariantList _rgChannelOptionEnabled;

    QVariantList _simpleModeEnabled;
    QVariantList _superSimpleModeEnabled;

    static constexpr uint8_t _allSimpleBits = 0x3F;
    static constexpr int _cChannelOptions = 11;
    static constexpr int _cSimpleModeBits = 8;
    static constexpr int _cFltModes = 6;

    static constexpr const char *_simpleParamName = "SIMPLE";
    static constexpr const char *_superSimpleParamName = "SUPER_SIMPLE";
};
