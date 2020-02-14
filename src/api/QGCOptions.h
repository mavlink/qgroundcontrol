/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QColor>

/// @file
/// @brief Core Plugin Interface for QGroundControl - Application Options
/// @author Gus Grubba <gus@auterion.com>

class CustomInstrumentWidget;
class QGCOptions : public QObject
{
    Q_OBJECT
public:
    QGCOptions(QObject* parent = nullptr);

    Q_PROPERTY(bool                     combineSettingsAndSetup         READ combineSettingsAndSetup        CONSTANT)
    Q_PROPERTY(double                   toolbarHeightMultiplier         READ toolbarHeightMultiplier        CONSTANT)
    Q_PROPERTY(bool                     enablePlanViewSelector          READ enablePlanViewSelector         CONSTANT)
    Q_PROPERTY(CustomInstrumentWidget*  instrumentWidget                READ instrumentWidget               CONSTANT)
    Q_PROPERTY(QUrl                     flyViewOverlay                  READ flyViewOverlay                 CONSTANT)
    Q_PROPERTY(QUrl                     preFlightChecklistUrl           READ preFlightChecklistUrl          CONSTANT)

    Q_PROPERTY(QUrl                     mainToolbarUrl                  READ mainToolbarUrl                 CONSTANT)
    Q_PROPERTY(QUrl                     planToolbarUrl                  READ planToolbarUrl                 CONSTANT)
    Q_PROPERTY(QColor                   toolbarBackgroundLight          READ toolbarBackgroundLight         CONSTANT)
    Q_PROPERTY(QColor                   toolbarBackgroundDark           READ toolbarBackgroundDark          CONSTANT)

    Q_PROPERTY(QUrl                     planToolbarIndicatorsUrl        READ planToolbarIndicatorsUrl       CONSTANT)
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
    Q_PROPERTY(bool                     guidedBarShowROI                READ guidedBarShowROI               NOTIFY guidedBarShowROIChanged)
    Q_PROPERTY(bool                     missionWaypointsOnly            READ missionWaypointsOnly           NOTIFY missionWaypointsOnlyChanged)
    Q_PROPERTY(bool                     multiVehicleEnabled             READ multiVehicleEnabled            NOTIFY multiVehicleEnabledChanged)
    Q_PROPERTY(bool                     showOfflineMapExport            READ showOfflineMapExport           NOTIFY showOfflineMapExportChanged)
    Q_PROPERTY(bool                     showOfflineMapImport            READ showOfflineMapImport           NOTIFY showOfflineMapImportChanged)
    Q_PROPERTY(bool                     useMobileFileDialog             READ useMobileFileDialog            CONSTANT)
    Q_PROPERTY(bool                     showMissionStatus               READ showMissionStatus              CONSTANT)
    Q_PROPERTY(bool                     guidedActionsRequireRCRSSI      READ guidedActionsRequireRCRSSI     CONSTANT)
    Q_PROPERTY(bool                     showMissionAbsoluteAltitude     READ showMissionAbsoluteAltitude    NOTIFY showMissionAbsoluteAltitudeChanged)
    Q_PROPERTY(bool                     showSimpleMissionStart          READ showSimpleMissionStart         NOTIFY showSimpleMissionStartChanged)
    Q_PROPERTY(bool                     disableVehicleConnection        READ disableVehicleConnection       CONSTANT)
    Q_PROPERTY(float                    devicePixelRatio                READ devicePixelRatio               NOTIFY devicePixelRatioChanged)
    Q_PROPERTY(float                    devicePixelDensity              READ devicePixelDensity             NOTIFY devicePixelDensityChanged)
    Q_PROPERTY(bool                     checkFirmwareVersion            READ checkFirmwareVersion           CONSTANT)
    Q_PROPERTY(bool                     showMavlinkLogOptions           READ showMavlinkLogOptions          CONSTANT)
    Q_PROPERTY(bool                     enableMultiVehicleList          READ enableMultiVehicleList         CONSTANT)
    Q_PROPERTY(bool                     enableMapScale                  READ enableMapScale                 CONSTANT)
    Q_PROPERTY(bool                     enableSaveMainWindowPosition    READ enableSaveMainWindowPosition   CONSTANT)

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

    /// Should the mission status indicator (Plan View) be shown?
    /// @return Yes or no
    virtual bool    showMissionStatus               () { return true; }

    /// Allows access to the full fly view window
    virtual QUrl    flyViewOverlay                  () const { return QUrl(); }

    /// Provides an optional, custom preflight checklist
    virtual QUrl    preFlightChecklistUrl           () const { return QUrl::fromUserInput("qrc:/qml/PreFlightCheckList.qml"); }

    /// Allows replacing the Main toolbar
    virtual QUrl    mainToolbarUrl                  () const;
    /// Allows replacing the Plan View toolbar
    virtual QUrl    planToolbarUrl                  () const;
    /// Allows replacing the toolbar Light Theme color
    virtual QColor  toolbarBackgroundLight          () const;
    /// Allows replacing the toolbar Dark Theme color
    virtual QColor  toolbarBackgroundDark           () const;
    /// Allows replacing the Plan View toolbar container
    virtual QUrl    planToolbarIndicatorsUrl        () const;
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
    virtual bool    guidedBarShowROI                () const { return true; }
    virtual bool    missionWaypointsOnly            () const { return false; }  ///< true: Only allow waypoints and complex items in Plan
    virtual bool    multiVehicleEnabled             () const { return true; }   ///< false: multi vehicle support is disabled
    virtual bool    guidedActionsRequireRCRSSI      () const { return false; }  ///< true: Guided actions will be disabled is there is no RC RSSI
    virtual bool    showOfflineMapExport            () const { return true; }
    virtual bool    showOfflineMapImport            () const { return true; }
    virtual bool    showMissionAbsoluteAltitude     () const { return true; }
    virtual bool    showSimpleMissionStart          () const { return false; }
    virtual bool    disableVehicleConnection        () const { return false; }  ///< true: vehicle connection is disabled
    virtual bool    checkFirmwareVersion            () const { return true; }
    virtual bool    showMavlinkLogOptions           () const { return true; }
    virtual bool    enableMultiVehicleList          () const { return true; }
    virtual bool    enableMapScale                  () const { return true; }
    /// Desktop builds save the main application size and position on close (and restore it on open)
    virtual bool    enableSaveMainWindowPosition    () const { return true; }

#if defined(__mobile__)
    virtual bool    useMobileFileDialog             () const { return true;}
#else
    virtual bool    useMobileFileDialog             () const { return false;}
#endif

    /// If returned QString in non-empty it means that firmware upgrade will run in a mode which only
    /// supports downloading a single firmware file from the URL. It also supports custom install through
    /// the Advanced options.
    virtual QString firmwareUpgradeSingleURL        () const { return QString(); }

    /// Device specific pixel ratio/density (for when Qt doesn't properly read it from the hardware)
    virtual float   devicePixelRatio                () const { return 0.0f; }
    virtual float   devicePixelDensity              () const { return 0.0f; }

signals:
    void showSensorCalibrationCompassChanged    (bool show);
    void showSensorCalibrationGyroChanged       (bool show);
    void showSensorCalibrationAccelChanged      (bool show);
    void showSensorCalibrationLevelChanged      (bool show);
    void showSensorCalibrationAirspeedChanged   (bool show);
    void showFirmwareUpgradeChanged             (bool show);
    void guidedBarShowEmergencyStopChanged      (bool show);
    void guidedBarShowOrbitChanged              (bool show);
    void guidedBarShowROIChanged                (bool show);
    void missionWaypointsOnlyChanged            (bool missionWaypointsOnly);
    void multiVehicleEnabledChanged             (bool multiVehicleEnabled);
    void showOfflineMapExportChanged            ();
    void showOfflineMapImportChanged            ();
    void showMissionAbsoluteAltitudeChanged     ();
    void showSimpleMissionStartChanged          ();
    void devicePixelRatioChanged                ();
    void devicePixelDensityChanged              ();

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
        POS_TOP_RIGHT,
        POS_CENTER_RIGHT,
        POS_BOTTOM_RIGHT,
        POS_TOP_LEFT,
        POS_CENTER_LEFT,
        POS_BOTTOM_LEFT
    };
    Q_ENUM(Pos)
    CustomInstrumentWidget(QObject* parent = nullptr);
    Q_PROPERTY(QUrl     source  READ source CONSTANT)
    Q_PROPERTY(Pos      widgetPosition              READ widgetPosition             NOTIFY widgetPositionChanged)
    virtual QUrl        source                      () { return QUrl(); }
    virtual Pos         widgetPosition              () { return POS_TOP_RIGHT; }
signals:
    void widgetPositionChanged  ();
};
