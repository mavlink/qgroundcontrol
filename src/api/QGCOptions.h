/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

/// @file
///     @brief Core Plugin Interface for QGroundControl - Application Options
///     @author Gus Grubba <mavlink@grubba.com>

class CustomInstrumentWidget;
class QGCOptions : public QObject
{
    Q_OBJECT
public:
    QGCOptions(QObject* parent = NULL);

    Q_PROPERTY(bool                     combineSettingsAndSetup         READ combineSettingsAndSetup        CONSTANT)
    Q_PROPERTY(double                   toolbarHeightMultiplier         READ toolbarHeightMultiplier        CONSTANT)
    Q_PROPERTY(bool                     enablePlanViewSelector          READ enablePlanViewSelector         CONSTANT)
    Q_PROPERTY(CustomInstrumentWidget*  instrumentWidget                READ instrumentWidget               CONSTANT)
    Q_PROPERTY(bool                     showSensorCalibrationCompass    READ showSensorCalibrationCompass   NOTIFY showSensorCalibrationCompassChanged)
    Q_PROPERTY(bool                     showSensorCalibrationGyro       READ showSensorCalibrationGyro      NOTIFY showSensorCalibrationGyroChanged)
    Q_PROPERTY(bool                     showSensorCalibrationAccel      READ showSensorCalibrationAccel     NOTIFY showSensorCalibrationAccelChanged)
    Q_PROPERTY(bool                     showSensorCalibrationLevel      READ showSensorCalibrationLevel     NOTIFY showSensorCalibrationLevelChanged)
    Q_PROPERTY(bool                     showSensorCalibrationAirspeed   READ showSensorCalibrationAirspeed  NOTIFY showSensorCalibrationAirspeedChanged)
    Q_PROPERTY(bool                     sensorsHaveFixedOrientation     READ sensorsHaveFixedOrientation    CONSTANT)
    Q_PROPERTY(bool                     wifiReliableForCalibration      READ wifiReliableForCalibration     CONSTANT)
    Q_PROPERTY(bool                     showFirmwareUpgrade             READ showFirmwareUpgrade            NOTIFY showFirmwareUpgradeChanged)
    Q_PROPERTY(QString                  firmwareUpgradeSingleURL        READ firmwareUpgradeSingleURL       CONSTANT)
    Q_PROPERTY(bool                     guidedBarShowEmergencyStop      READ guidedBarShowEmergencyStop     NOTIFY guidedBarShowEmergencyStopChanged)
    Q_PROPERTY(bool                     guidedBarShowOrbit              READ guidedBarShowOrbit             NOTIFY guidedBarShowOrbitChanged)

    /// Should QGC hide its settings menu and colapse it into one single menu (Settings and Vehicle Setup)?
    /// @return true if QGC should consolidate both menus into one.
    virtual bool        combineSettingsAndSetup     () { return false; }

    /// Main ToolBar Multiplier.
    /// @return Factor to use when computing toolbar height
    virtual double      toolbarHeightMultiplier     () { return 1.0; }

    /// Enable Plan View Selector (Mission, Fence or Rally)
    /// @return True or false
    virtual bool        enablePlanViewSelector      () { return true; }

    /// Provides an alternate instrument widget for the Fly View
    /// @return An alternate widget (see QGCInstrumentWidget.qml, the default widget)
    virtual CustomInstrumentWidget* instrumentWidget();

    /// By returning false you can hide the following sensor calibration pages
    virtual bool    showSensorCalibrationCompass    () const { return true; }
    virtual bool    showSensorCalibrationGyro       () const { return true; }
    virtual bool    showSensorCalibrationAccel      () const { return true; }
    virtual bool    showSensorCalibrationLevel      () const { return true; }
    virtual bool    showSensorCalibrationAirspeed   () const { return true; }
    virtual bool    wifiReliableForCalibration      () const { return false; }
    virtual bool    sensorsHaveFixedOrientation     () const { return false; }

    virtual bool    showFirmwareUpgrade             () const { return true; }

    virtual bool    guidedBarShowEmergencyStop      () const { return true; }
    virtual bool    guidedBarShowOrbit              () const { return true; }

    /// If returned QString in non-empty it means that firmware upgrade will run in a mode which only
    /// supports downloading a single firmware file from the URL. It also supports custom install through
    /// the Advanced options.
    virtual QString firmwareUpgradeSingleURL        () const { return QString(); }

signals:
    void showSensorCalibrationCompassChanged    (bool show);
    void showSensorCalibrationGyroChanged       (bool show);
    void showSensorCalibrationAccelChanged      (bool show);
    void showSensorCalibrationLevelChanged      (bool show);
    void showSensorCalibrationAirspeedChanged   (bool show);
    void showFirmwareUpgradeChanged             (bool show);
    void guidedBarShowEmergencyStopChanged       (bool show);
    void guidedBarShowOrbitChanged              (bool show);

private:
    CustomInstrumentWidget* _defaultInstrumentWidget;
};

//-----------------------------------------------------------------------------
class CustomInstrumentWidget : public QObject
{
    Q_OBJECT
public:
    //-- Widget Position
    enum Pos {
        POS_TOP_RIGHT           = 0,
        POS_CENTER_RIGHT        = 1,
        POS_BOTTOM_RIGHT        = 2,
    };
    Q_ENUMS(Pos)
    CustomInstrumentWidget(QObject* parent = NULL);
    Q_PROPERTY(QUrl     source  READ source CONSTANT)
    Q_PROPERTY(Pos      widgetPosition              READ widgetPosition             NOTIFY widgetPositionChanged)
    virtual QUrl        source                      () { return QUrl(); }
    virtual Pos         widgetPosition              () { return POS_CENTER_RIGHT; }
signals:
    void widgetPositionChanged  ();
};
