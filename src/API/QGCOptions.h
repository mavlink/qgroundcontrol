/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtGui/QColor>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

class QGCOptions;

Q_DECLARE_LOGGING_CATEGORY(QGCFlyViewOptionsLog)

class QGCFlyViewOptions : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_PROPERTY(bool showMultiVehicleList        READ showMultiVehicleList       CONSTANT)
    Q_PROPERTY(bool showInstrumentPanel         READ showInstrumentPanel        CONSTANT)
    Q_PROPERTY(bool showMapScale                READ showMapScale               CONSTANT)
    Q_PROPERTY(bool guidedBarShowEmergencyStop  READ guidedBarShowEmergencyStop NOTIFY guidedBarShowEmergencyStopChanged)
    Q_PROPERTY(bool guidedBarShowOrbit          READ guidedBarShowOrbit         NOTIFY guidedBarShowOrbitChanged)
    Q_PROPERTY(bool guidedBarShowROI            READ guidedBarShowROI           NOTIFY guidedBarShowROIChanged)

public:
    explicit QGCFlyViewOptions(QGCOptions *options, QObject *parent = nullptr);
    ~QGCFlyViewOptions();

signals:
    void guidedBarShowEmergencyStopChanged(bool show);
    void guidedBarShowOrbitChanged(bool show);
    void guidedBarShowROIChanged(bool show);

protected:
    virtual bool showMultiVehicleList() const { return true; }
    virtual bool showMapScale() const { return true; }
    virtual bool showInstrumentPanel() const { return true; }
    virtual bool guidedBarShowEmergencyStop() const { return true; }
    virtual bool guidedBarShowOrbit() const { return true; }
    virtual bool guidedBarShowROI() const { return true; }

    const QGCOptions *_options = nullptr;
};

/*===========================================================================*/

Q_DECLARE_LOGGING_CATEGORY(QGCOptionsLog)

class QGCOptions : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_PROPERTY(bool allowJoystickSelection          READ allowJoystickSelection         NOTIFY allowJoystickSelectionChanged)
    Q_PROPERTY(bool checkFirmwareVersion            READ checkFirmwareVersion           CONSTANT)
    Q_PROPERTY(bool combineSettingsAndSetup         READ combineSettingsAndSetup        CONSTANT)
    Q_PROPERTY(bool disableVehicleConnection        READ disableVehicleConnection       CONSTANT)
    Q_PROPERTY(bool enablePlanViewSelector          READ enablePlanViewSelector         CONSTANT)
    Q_PROPERTY(bool enableSaveMainWindowPosition    READ enableSaveMainWindowPosition   CONSTANT)
    Q_PROPERTY(bool guidedActionsRequireRCRSSI      READ guidedActionsRequireRCRSSI     CONSTANT)
    Q_PROPERTY(bool missionWaypointsOnly            READ missionWaypointsOnly           NOTIFY missionWaypointsOnlyChanged)
    Q_PROPERTY(bool multiVehicleEnabled             READ multiVehicleEnabled            NOTIFY multiVehicleEnabledChanged)
    Q_PROPERTY(bool sensorsHaveFixedOrientation     READ sensorsHaveFixedOrientation    CONSTANT)
    Q_PROPERTY(bool showFirmwareUpgrade             READ showFirmwareUpgrade            NOTIFY showFirmwareUpgradeChanged)
    Q_PROPERTY(bool showMissionAbsoluteAltitude     READ showMissionAbsoluteAltitude    NOTIFY showMissionAbsoluteAltitudeChanged)
    Q_PROPERTY(bool showMissionStatus               READ showMissionStatus              CONSTANT)
    Q_PROPERTY(bool showOfflineMapExport            READ showOfflineMapExport           NOTIFY showOfflineMapExportChanged)
    Q_PROPERTY(bool showOfflineMapImport            READ showOfflineMapImport           NOTIFY showOfflineMapImportChanged)
    Q_PROPERTY(bool showPX4LogTransferOptions       READ showPX4LogTransferOptions      CONSTANT)
    Q_PROPERTY(bool showSensorCalibrationAccel      READ showSensorCalibrationAccel     NOTIFY showSensorCalibrationAccelChanged)
    Q_PROPERTY(bool showSensorCalibrationAirspeed   READ showSensorCalibrationAirspeed  NOTIFY showSensorCalibrationAirspeedChanged)
    Q_PROPERTY(bool showSensorCalibrationCompass    READ showSensorCalibrationCompass   NOTIFY showSensorCalibrationCompassChanged)
    Q_PROPERTY(bool showSensorCalibrationGyro       READ showSensorCalibrationGyro      NOTIFY showSensorCalibrationGyroChanged)
    Q_PROPERTY(bool showSensorCalibrationLevel      READ showSensorCalibrationLevel     NOTIFY showSensorCalibrationLevelChanged)
    Q_PROPERTY(bool showSimpleMissionStart          READ showSimpleMissionStart         NOTIFY showSimpleMissionStartChanged)
    Q_PROPERTY(bool useMobileFileDialog             READ useMobileFileDialog            CONSTANT)
    Q_PROPERTY(bool wifiReliableForCalibration      READ wifiReliableForCalibration     CONSTANT)
    Q_PROPERTY(double toolbarHeightMultiplier       READ toolbarHeightMultiplier        CONSTANT)
    Q_PROPERTY(float devicePixelDensity             READ devicePixelDensity             NOTIFY devicePixelDensityChanged)
    Q_PROPERTY(float devicePixelRatio               READ devicePixelRatio               NOTIFY devicePixelRatioChanged)
    Q_PROPERTY(const QGCFlyViewOptions *flyView     READ flyViewOptions                 CONSTANT)
    Q_PROPERTY(QString firmwareUpgradeSingleURL     READ firmwareUpgradeSingleURL       CONSTANT)
    Q_PROPERTY(QStringList surveyBuiltInPresetNames READ surveyBuiltInPresetNames       CONSTANT)
    Q_PROPERTY(QUrl preFlightChecklistUrl           READ preFlightChecklistUrl          CONSTANT)

public:
    explicit QGCOptions(QObject *parent = nullptr);
    ~QGCOptions();

    /// Should QGC hide its settings menu and colapse it into one single menu (Settings and Vehicle Setup)?
    /// @return true if QGC should consolidate both menus into one.
    virtual bool combineSettingsAndSetup() const { return false; }

    /// Main ToolBar Multiplier.
    /// @return Factor to use when computing toolbar height
    virtual double toolbarHeightMultiplier() const { return 1.0; }

    /// Enable Plan View Selector (Mission, Fence or Rally)
    /// @return True or false
    virtual bool enablePlanViewSelector() const { return true; }

    /// Should the mission status indicator (Plan View) be shown?
    /// @return Yes or no
    virtual bool showMissionStatus() const { return true; }

    /// Provides an optional, custom preflight checklist
    virtual QUrl preFlightChecklistUrl() const { return QUrl::fromUserInput(QStringLiteral("qrc:/qml/PreFlightCheckList.qml")); }

    /// Allows replacing the toolbar Light Theme color
    virtual QColor toolbarBackgroundLight() const { return QColorConstants::White; }

    /// Allows replacing the toolbar Dark Theme color
    virtual QColor toolbarBackgroundDark() const { return QColorConstants::Black; }

    /// By returning false you can hide the following sensor calibration pages
    virtual bool showSensorCalibrationAccel() const { return true; }
    virtual bool showSensorCalibrationAirspeed() const { return true; }
    virtual bool showSensorCalibrationCompass() const { return true; }
    virtual bool showSensorCalibrationGyro() const { return true; }
    virtual bool showSensorCalibrationLevel() const { return true; }

    /// @return false: custom build has automatically enabled a specific joystick
    virtual bool allowJoystickSelection() const { return true; }

    virtual bool checkFirmwareVersion() const { return true; }

    /// @return true: vehicle connection is disabled
    virtual bool disableVehicleConnection() const { return false; }

    /// @return true: Guided actions will be disabled is there is no RC RSSI
    virtual bool guidedActionsRequireRCRSSI() const { return false; }

    /// @return true: Only allow waypoints and complex items in Plan
    virtual bool missionWaypointsOnly() const { return false; }

    /// @return false: multi vehicle support is disabled
    virtual bool multiVehicleEnabled() const { return true; }

    virtual bool sensorsHaveFixedOrientation() const { return false; }

    virtual bool showFirmwareUpgrade() const { return true; }
    virtual bool showMissionAbsoluteAltitude() const { return true; }
    virtual bool showOfflineMapExport() const { return true; }
    virtual bool showOfflineMapImport() const { return true; }
    virtual bool showPX4LogTransferOptions() const { return true; }
    virtual bool showSimpleMissionStart() const { return false; }

    virtual bool wifiReliableForCalibration() const { return false; }

    /// Desktop builds save the main application size and position on close (and restore it on open)
    virtual bool enableSaveMainWindowPosition() const { return true; }

    virtual QStringList surveyBuiltInPresetNames() const { return QStringList(); } ///< Built in presets cannot be deleted

#if defined (Q_OS_ANDROID) || defined(Q_OS_IOS)
    virtual bool useMobileFileDialog() const { return true; }
#else
    virtual bool useMobileFileDialog() const { return false; }
#endif

    /// If returned QString in non-empty it means that firmware upgrade will run in a mode which only
    /// supports downloading a single firmware file from the URL. It also supports custom install through
    /// the Advanced options.
    virtual QString firmwareUpgradeSingleURL() const { return QString(); }

    /// Device specific pixel ratio/density (for when Qt doesn't properly read it from the hardware)
    virtual float devicePixelRatio() const { return 0.0f; }
    virtual float devicePixelDensity() const { return 0.0f; }

    virtual const QGCFlyViewOptions *flyViewOptions() const { return _defaultFlyViewOptions; }

signals:
    void allowJoystickSelectionChanged(bool allow);
    void devicePixelDensityChanged();
    void devicePixelRatioChanged();
    void missionWaypointsOnlyChanged(bool missionWaypointsOnly);
    void multiVehicleEnabledChanged(bool multiVehicleEnabled);
    void showFirmwareUpgradeChanged(bool show);
    void showMissionAbsoluteAltitudeChanged();
    void showOfflineMapExportChanged();
    void showOfflineMapImportChanged();
    void showSensorCalibrationAccelChanged(bool show);
    void showSensorCalibrationAirspeedChanged(bool show);
    void showSensorCalibrationCompassChanged(bool show);
    void showSensorCalibrationGyroChanged(bool show);
    void showSensorCalibrationLevelChanged(bool show);
    void showSimpleMissionStartChanged();

protected:
    const QGCFlyViewOptions *_defaultFlyViewOptions = nullptr;
};
