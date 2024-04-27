/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QmlUnitsConversion.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>
#include <QtCore/QPointF>
#include <QtPositioning/QGeoCoordinate>

class QGCToolbox;
class LinkManager;
class FactGroup;
class QGCPalette;
class AirLinkManager;
class UTMSPManager;
class ADSBVehicleManager;
class MultiVehicleManager;
class QGCPositionManager;
class SettingsManager;
class QGCCorePlugin;
class MissionCommandTree;
class QGCApplication;

Q_MOC_INCLUDE("QGCPalette.h")
Q_MOC_INCLUDE("FactGroup.h")
Q_MOC_INCLUDE("LinkManager.h")
Q_MOC_INCLUDE("QGCMapEngineManager.h")
Q_MOC_INCLUDE("PositionManager.h")
Q_MOC_INCLUDE("VideoManager.h")
Q_MOC_INCLUDE("MAVLinkLogManager.h")
Q_MOC_INCLUDE("SettingsManager.h")
Q_MOC_INCLUDE("QGCCorePlugin.h")
Q_MOC_INCLUDE("MissionCommandTree.h")
Q_MOC_INCLUDE("ADSBVehicleManager.h")
Q_MOC_INCLUDE("MultiVehicleManager.h")
Q_MOC_INCLUDE("PositionManager.h")
#ifdef CONFIG_UTM_ADAPTER
Q_MOC_INCLUDE("UTMSPManager.h")
#endif
#ifndef QGC_AIRLINK_DISABLED
Q_MOC_INCLUDE("AirLinkManager.h")
#endif

class QGroundControlQmlGlobal : public QGCTool
{
    Q_OBJECT

public:
    QGroundControlQmlGlobal(QGCApplication* app, QGCToolbox* toolbox);
    ~QGroundControlQmlGlobal();

    enum AltMode {
        AltitudeModeMixed,              // Used by global altitude mode for mission planning
        AltitudeModeRelative,           // MAV_FRAME_GLOBAL_RELATIVE_ALT
        AltitudeModeAbsolute,           // MAV_FRAME_GLOBAL
        AltitudeModeCalcAboveTerrain,   // Absolute altitude above terrain calculated from terrain data
        AltitudeModeTerrainFrame,       // MAV_FRAME_GLOBAL_TERRAIN_ALT
        AltitudeModeNone,               // Being used as distance value unrelated to ground (for example distance to structure)
    };
    Q_ENUM(AltMode)

    Q_PROPERTY(QString              appName                 READ    appName                 CONSTANT)
    Q_PROPERTY(LinkManager*         linkManager             READ    linkManager             CONSTANT)
    Q_PROPERTY(MultiVehicleManager* multiVehicleManager     READ    multiVehicleManager     CONSTANT)
    Q_PROPERTY(QGCMapEngineManager* mapEngineManager        READ    mapEngineManager        CONSTANT)
    Q_PROPERTY(QGCPositionManager*  qgcPositionManger       READ    qgcPositionManger       CONSTANT)
    Q_PROPERTY(VideoManager*        videoManager            READ    videoManager            CONSTANT)
    Q_PROPERTY(MAVLinkLogManager*   mavlinkLogManager       READ    mavlinkLogManager       CONSTANT)
    Q_PROPERTY(SettingsManager*     settingsManager         READ    settingsManager         CONSTANT)
    Q_PROPERTY(ADSBVehicleManager*  adsbVehicleManager      READ    adsbVehicleManager      CONSTANT)
    Q_PROPERTY(QGCCorePlugin*       corePlugin              READ    corePlugin              CONSTANT)
    Q_PROPERTY(MissionCommandTree*  missionCommandTree      READ    missionCommandTree      CONSTANT)
    Q_PROPERTY(FactGroup*           gpsRtk                  READ    gpsRtkFactGroup         CONSTANT)
#ifndef QGC_AIRLINK_DISABLED
    Q_PROPERTY(AirLinkManager*      airlinkManager          READ    airlinkManager          CONSTANT)
#endif
    Q_PROPERTY(bool                 airlinkSupported        READ    airlinkSupported        CONSTANT)
    Q_PROPERTY(QGCPalette*          globalPalette           MEMBER  _globalPalette          CONSTANT)   ///< This palette will always return enabled colors
    Q_PROPERTY(QmlUnitsConversion*  unitsConversion         READ    unitsConversion         CONSTANT)
    Q_PROPERTY(bool                 singleFirmwareSupport   READ    singleFirmwareSupport   CONSTANT)
    Q_PROPERTY(bool                 singleVehicleSupport    READ    singleVehicleSupport    CONSTANT)
    Q_PROPERTY(bool                 px4ProFirmwareSupported READ    px4ProFirmwareSupported CONSTANT)
    Q_PROPERTY(int                  apmFirmwareSupported    READ    apmFirmwareSupported    CONSTANT)
    Q_PROPERTY(QGeoCoordinate       flightMapPosition       READ    flightMapPosition       WRITE setFlightMapPosition  NOTIFY flightMapPositionChanged)
    Q_PROPERTY(double               flightMapZoom           READ    flightMapZoom           WRITE setFlightMapZoom      NOTIFY flightMapZoomChanged)
    Q_PROPERTY(double               flightMapInitialZoom    MEMBER  _flightMapInitialZoom   CONSTANT)   ///< Zoom level to use when either gcs or vehicle shows up for first time

    Q_PROPERTY(QString  parameterFileExtension  READ parameterFileExtension CONSTANT)
    Q_PROPERTY(QString  missionFileExtension    READ missionFileExtension   CONSTANT)
    Q_PROPERTY(QString  telemetryFileExtension  READ telemetryFileExtension CONSTANT)

    Q_PROPERTY(QString qgcVersion       READ qgcVersion         CONSTANT)
    Q_PROPERTY(bool    skipSetupPage    READ skipSetupPage      WRITE setSkipSetupPage NOTIFY skipSetupPageChanged)

    Q_PROPERTY(qreal zOrderTopMost              READ zOrderTopMost              CONSTANT) ///< z order for top most items, toolbar, main window sub view
    Q_PROPERTY(qreal zOrderWidgets              READ zOrderWidgets              CONSTANT) ///< z order value to widgets, for example: zoom controls, hud widgetss
    Q_PROPERTY(qreal zOrderMapItems             READ zOrderMapItems             CONSTANT)
    Q_PROPERTY(qreal zOrderVehicles             READ zOrderVehicles             CONSTANT)
    Q_PROPERTY(qreal zOrderWaypointIndicators   READ zOrderWaypointIndicators   CONSTANT)
    Q_PROPERTY(qreal zOrderTrajectoryLines      READ zOrderTrajectoryLines      CONSTANT)
    Q_PROPERTY(qreal zOrderWaypointLines        READ zOrderWaypointLines        CONSTANT)
    //-------------------------------------------------------------------------
    // MavLink Protocol
    Q_PROPERTY(bool     isVersionCheckEnabled   READ isVersionCheckEnabled      WRITE setIsVersionCheckEnabled      NOTIFY isVersionCheckEnabledChanged)
    Q_PROPERTY(int      mavlinkSystemID         READ mavlinkSystemID            WRITE setMavlinkSystemID            NOTIFY mavlinkSystemIDChanged)
    Q_PROPERTY(bool     hasAPMSupport           READ hasAPMSupport              CONSTANT)
    Q_PROPERTY(bool     hasMAVLinkInspector     READ hasMAVLinkInspector        CONSTANT)


    //-------------------------------------------------------------------------
    // Elevation Provider
    Q_PROPERTY(QString  elevationProviderName           READ elevationProviderName              CONSTANT)
    Q_PROPERTY(QString  elevationProviderNotice         READ elevationProviderNotice            CONSTANT)

    Q_PROPERTY(bool              utmspSupported           READ    utmspSupported              CONSTANT)

#ifdef CONFIG_UTM_ADAPTER
    Q_PROPERTY(UTMSPManager*     utmspManager             READ    utmspManager                CONSTANT)
#endif

    Q_INVOKABLE void    saveGlobalSetting       (const QString& key, const QString& value);
    Q_INVOKABLE QString loadGlobalSetting       (const QString& key, const QString& defaultValue);
    Q_INVOKABLE void    saveBoolGlobalSetting   (const QString& key, bool value);
    Q_INVOKABLE bool    loadBoolGlobalSetting   (const QString& key, bool defaultValue);

    Q_INVOKABLE void    deleteAllSettingsNextBoot       ();
    Q_INVOKABLE void    clearDeleteAllSettingsNextBoot  ();

    Q_INVOKABLE void    startPX4MockLink            (bool sendStatusText);
    Q_INVOKABLE void    startGenericMockLink        (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduCopterMockLink  (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduPlaneMockLink   (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduSubMockLink     (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduRoverMockLink   (bool sendStatusText);
    Q_INVOKABLE void    stopOneMockLink             (void);

    /// Returns the list of available logging category names.
    Q_INVOKABLE QStringList loggingCategories(void) const { return QGCLoggingCategoryRegister::instance()->registeredCategories(); }

    /// Turns on/off logging for the specified category. State is saved in app settings.
    Q_INVOKABLE void setCategoryLoggingOn(const QString& category, bool enable) { QGCLoggingCategoryRegister::instance()->setCategoryLoggingOn(category, enable); }

    /// Returns true if logging is turned on for the specified category.
    Q_INVOKABLE bool categoryLoggingOn(const QString& category) { return QGCLoggingCategoryRegister::instance()->categoryLoggingOn(category); }

    /// Updates the logging filter rules after settings have changed
    Q_INVOKABLE void updateLoggingFilterRules(void) { QGCLoggingCategoryRegister::instance()->setFilterRulesFromSettings(QString()); }

    Q_INVOKABLE bool linesIntersect(QPointF xLine1, QPointF yLine1, QPointF xLine2, QPointF yLine2);

    Q_INVOKABLE QString altitudeModeExtraUnits(AltMode altMode);        ///< String shown in the FactTextField.extraUnits ui
    Q_INVOKABLE QString altitudeModeShortDescription(AltMode altMode);  ///< String shown when a user needs to select an altitude mode

    // Property accesors

    QString                 appName             ();
    LinkManager*            linkManager         ()  { return _linkManager; }
    MultiVehicleManager*    multiVehicleManager ()  { return _multiVehicleManager; }
    QGCMapEngineManager*    mapEngineManager    ()  { return _mapEngineManager; }
    QGCPositionManager*     qgcPositionManger   ()  { return _qgcPositionManager; }
    MissionCommandTree*     missionCommandTree  ()  { return _missionCommandTree; }
    VideoManager*           videoManager        ()  { return _videoManager; }
    MAVLinkLogManager*      mavlinkLogManager   ()  { return _mavlinkLogManager; }
    QGCCorePlugin*          corePlugin          ()  { return _corePlugin; }
    SettingsManager*        settingsManager     ()  { return _settingsManager; }
    FactGroup*              gpsRtkFactGroup     ()  { return _gpsRtkFactGroup; }
    ADSBVehicleManager*     adsbVehicleManager  ()  { return _adsbVehicleManager; }
    QmlUnitsConversion*     unitsConversion     ()  { return &_unitsConversion; }
    static QGeoCoordinate   flightMapPosition   ()  { return _coord; }
    static double           flightMapZoom       ()  { return _zoom; }

    AirLinkManager*         airlinkManager      ()  { return _airlinkManager; }
#ifndef QGC_AIRLINK_DISABLED
    bool                    airlinkSupported    ()  { return true; }
#else
    bool                    airlinkSupported    () { return false; }
#endif

#ifdef CONFIG_UTM_ADAPTER
    UTMSPManager*            utmspManager         ()  {return _utmspManager;}
#endif

    qreal zOrderTopMost             () { return 1000; }
    qreal zOrderWidgets             () { return 100; }
    qreal zOrderMapItems            () { return 50; }
    qreal zOrderWaypointIndicators  () { return 50; }
    qreal zOrderVehicles            () { return 49; }
    qreal zOrderTrajectoryLines     () { return 48; }
    qreal zOrderWaypointLines       () { return 47; }

    bool    isVersionCheckEnabled   ();
    int     mavlinkSystemID         ();
#if defined(NO_ARDUPILOT_DIALECT)
    bool    hasAPMSupport           () { return false; }
#else
    bool    hasAPMSupport           () { return true; }
#endif

#if defined(QGC_DISABLE_MAVLINK_INSPECTOR)
    bool    hasMAVLinkInspector     () { return false; }
#else
    bool    hasMAVLinkInspector     () { return true; }
#endif

    QString elevationProviderName   ();
    QString elevationProviderNotice ();

    bool    singleFirmwareSupport   ();
    bool    singleVehicleSupport    ();
    bool    px4ProFirmwareSupported ();
    bool    apmFirmwareSupported    ();
    bool    skipSetupPage           () const{ return _skipSetupPage; }
    void    setSkipSetupPage        (bool skip);

    void    setIsVersionCheckEnabled    (bool enable);
    void    setMavlinkSystemID          (int  id);
    void    setFlightMapPosition        (QGeoCoordinate& coordinate);
    void    setFlightMapZoom            (double zoom);

    QString parameterFileExtension  (void) const;
    QString missionFileExtension    (void) const;
    QString telemetryFileExtension  (void) const;

    QString qgcVersion              (void) const;

#ifdef CONFIG_UTM_ADAPTER
    bool    utmspSupported() { return true; }
#else
    bool    utmspSupported() { return false; }
#endif

    // Overrides from QGCTool
    virtual void setToolbox(QGCToolbox* toolbox);

signals:
    void isMultiplexingEnabledChanged   (bool enabled);
    void isVersionCheckEnabledChanged   (bool enabled);
    void mavlinkSystemIDChanged         (int id);
    void flightMapPositionChanged       (QGeoCoordinate flightMapPosition);
    void flightMapZoomChanged           (double flightMapZoom);
    void skipSetupPageChanged           ();

private:
    double                  _flightMapInitialZoom   = 17.0;
    LinkManager*            _linkManager            = nullptr;
    MultiVehicleManager*    _multiVehicleManager    = nullptr;
    QGCMapEngineManager*    _mapEngineManager       = nullptr;
    QGCPositionManager*     _qgcPositionManager     = nullptr;
    MissionCommandTree*     _missionCommandTree     = nullptr;
    VideoManager*           _videoManager           = nullptr;
    MAVLinkLogManager*      _mavlinkLogManager      = nullptr;
    QGCCorePlugin*          _corePlugin             = nullptr;
    FirmwarePluginManager*  _firmwarePluginManager  = nullptr;
    SettingsManager*        _settingsManager        = nullptr;
    FactGroup*              _gpsRtkFactGroup        = nullptr;
    AirLinkManager*         _airlinkManager         = nullptr;
    ADSBVehicleManager*     _adsbVehicleManager     = nullptr;
    QGCPalette*             _globalPalette          = nullptr;
    QmlUnitsConversion      _unitsConversion;
#ifdef CONFIG_UTM_ADAPTER
    UTMSPManager*            _utmspManager;
#endif

    bool                    _skipSetupPage          = false;
    QStringList             _altitudeModeEnumString;

    static const char* _flightMapPositionSettingsGroup;
    static const char* _flightMapPositionLatitudeSettingsKey;
    static const char* _flightMapPositionLongitudeSettingsKey;
    static const char* _flightMapZoomSettingsKey;

    static QGeoCoordinate   _coord;
    static double           _zoom;
    QTimer                  _flightMapPositionSettledTimer;
};
