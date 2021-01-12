/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMFlightModesComponentController_H
#define APMFlightModesComponentController_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QStringList>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"
#include "Vehicle.h"

/// MVC Controller for FlightModesComponent.qml.
class APMFlightModesComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    enum SimpleModeValues {
        SimpleModeStandard = 0,
        SimpleModeSimple,
        SimpleModeSuperSimple,
        SimpleModeCustom
    };
    Q_ENUM(SimpleModeValues)

    APMFlightModesComponentController(void);
    
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

    Q_INVOKABLE void setSimpleMode(int fltModeIndex, bool enabled);
    Q_INVOKABLE void setSuperSimpleMode(int fltModeIndex, bool enabled);

    int activeFlightMode(void) const { return _activeFlightMode; }
    QVariantList channelOptionEnabled(void) const { return _rgChannelOptionEnabled; }

signals:
    void activeFlightModeChanged        (int activeFlightMode);
    void channelOptionEnabledChanged    (void);
    void simpleModeChanged              (int simpleMode);
    void simpleModeEnabledChanged       (void);
    void superSimpleModeEnabledChanged  (void);

private slots:
    void _rcChannelsChanged                     (int channelCount, int pwmValues[Vehicle::cMaxRcChannels]);
    void _updateSimpleParamsFromSimpleMode      (void);
    void _setupSimpleModeEnabled     (void);

private:
    QString         _modeParamPrefix;
    QString         _modeChannelParam;
    int             _activeFlightMode;
    int             _channelCount;
    QVariantList    _rgChannelOptionEnabled;
    QStringList     _simpleModeNames;
    int             _simpleMode;
    Fact*           _simpleModeFact;
    Fact*           _superSimpleModeFact;
    bool            _simpleModesSupported;
    QVariantList    _simpleModeEnabled;
    QVariantList    _superSimpleModeEnabled;

    static const uint8_t    _allSimpleBits =    0x3F;
    static const int        _cChannelOptions =  11;
    static const int        _cSimpleModeBits =  8;
    static const int        _cFltModes =        6;

    static const char*      _simpleParamName;
    static const char*      _superSimpleParamName;

    static bool _typeRegistered;
};

#endif
