/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef QGroundControlQmlGlobal_H
#define QGroundControlQmlGlobal_H

#include "QGCToolbox.h"
#include "QGCApplication.h"
#include "LinkManager.h"
#include "FlightMapSettings.h"
#include "SettingsFact.h"
#include "FactMetaData.h"
#include "SimulatedPosition.h"
#include "QGCLoggingCategory.h"

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

class QGCToolbox;

class QGroundControlQmlGlobal : public QGCTool
{
    Q_OBJECT

public:
    QGroundControlQmlGlobal(QGCApplication* app);
    ~QGroundControlQmlGlobal();

    enum DistanceUnits {
        DistanceUnitsFeet = 0,
        DistanceUnitsMeters
    };

    enum AreaUnits {
        AreaUnitsSquareFeet = 0,
        AreaUnitsSquareMeters,
        AreaUnitsSquareKilometers,
        AreaUnitsHectares,
        AreaUnitsAcres,
        AreaUnitsSquareMiles,
    };

    enum SpeedUnits {
        SpeedUnitsFeetPerSecond = 0,
        SpeedUnitsMetersPerSecond,
        SpeedUnitsMilesPerHour,
        SpeedUnitsKilometersPerHour,
        SpeedUnitsKnots,
    };

    Q_ENUMS(DistanceUnits)
    Q_ENUMS(AreaUnits)
    Q_ENUMS(SpeedUnits)

    Q_PROPERTY(FlightMapSettings*   flightMapSettings   READ flightMapSettings      CONSTANT)
    Q_PROPERTY(LinkManager*         linkManager         READ linkManager            CONSTANT)
    Q_PROPERTY(MultiVehicleManager* multiVehicleManager READ multiVehicleManager    CONSTANT)
    Q_PROPERTY(QGCMapEngineManager* mapEngineManager    READ mapEngineManager       CONSTANT)
    Q_PROPERTY(QGCPositionManager*  qgcPositionManger   READ qgcPositionManger      CONSTANT)
    Q_PROPERTY(MissionCommandTree*  missionCommandTree  READ missionCommandTree     CONSTANT)
    Q_PROPERTY(VideoManager*        videoManager        READ videoManager           CONSTANT)
    Q_PROPERTY(MAVLinkLogManager*   mavlinkLogManager   READ mavlinkLogManager      CONSTANT)
    Q_PROPERTY(QGCCorePlugin*       corePlugin          READ corePlugin             CONSTANT)

    Q_PROPERTY(qreal                zOrderTopMost       READ zOrderTopMost          CONSTANT) ///< z order for top most items, toolbar, main window sub view
    Q_PROPERTY(qreal                zOrderWidgets       READ zOrderWidgets          CONSTANT) ///< z order value to widgets, for example: zoom controls, hud widgetss
    Q_PROPERTY(qreal                zOrderMapItems      READ zOrderMapItems         CONSTANT) ///< z order value for map items, for example: mission item indicators

    // Various QGC settings exposed to Qml
    Q_PROPERTY(bool     isDarkStyle             READ isDarkStyle                WRITE setIsDarkStyle                NOTIFY isDarkStyleChanged)              // TODO: Should be in ScreenTools?
    Q_PROPERTY(bool     isAudioMuted            READ isAudioMuted               WRITE setIsAudioMuted               NOTIFY isAudioMutedChanged)
    Q_PROPERTY(bool     isSaveLogPrompt         READ isSaveLogPrompt            WRITE setIsSaveLogPrompt            NOTIFY isSaveLogPromptChanged)
    Q_PROPERTY(bool     isSaveLogPromptNotArmed READ isSaveLogPromptNotArmed    WRITE setIsSaveLogPromptNotArmed    NOTIFY isSaveLogPromptNotArmedChanged)
    Q_PROPERTY(bool     virtualTabletJoystick   READ virtualTabletJoystick      WRITE setVirtualTabletJoystick      NOTIFY virtualTabletJoystickChanged)
    Q_PROPERTY(qreal    baseFontPointSize       READ baseFontPointSize          WRITE setBaseFontPointSize          NOTIFY baseFontPointSizeChanged)

    //-------------------------------------------------------------------------
    // MavLink Protocol
    Q_PROPERTY(bool     isVersionCheckEnabled   READ isVersionCheckEnabled      WRITE setIsVersionCheckEnabled      NOTIFY isVersionCheckEnabledChanged)
    Q_PROPERTY(int      mavlinkSystemID         READ mavlinkSystemID            WRITE setMavlinkSystemID            NOTIFY mavlinkSystemIDChanged)

    Q_PROPERTY(Fact*    offlineEditingFirmwareType      READ offlineEditingFirmwareType         CONSTANT)
    Q_PROPERTY(Fact*    offlineEditingVehicleType       READ offlineEditingVehicleType          CONSTANT)
    Q_PROPERTY(Fact*    offlineEditingCruiseSpeed       READ offlineEditingCruiseSpeed          CONSTANT)
    Q_PROPERTY(Fact*    offlineEditingHoverSpeed        READ offlineEditingHoverSpeed           CONSTANT)
    Q_PROPERTY(Fact*    distanceUnits                   READ distanceUnits                      CONSTANT)
    Q_PROPERTY(Fact*    areaUnits                       READ areaUnits                          CONSTANT)
    Q_PROPERTY(Fact*    speedUnits                      READ speedUnits                         CONSTANT)
    Q_PROPERTY(Fact*    batteryPercentRemainingAnnounce READ batteryPercentRemainingAnnounce    CONSTANT)

    Q_PROPERTY(QGeoCoordinate lastKnownHomePosition READ lastKnownHomePosition  CONSTANT)
    Q_PROPERTY(QGeoCoordinate flightMapPosition     MEMBER _flightMapPosition   NOTIFY flightMapPositionChanged)
    Q_PROPERTY(double         flightMapZoom         MEMBER _flightMapZoom       NOTIFY flightMapZoomChanged)

    Q_PROPERTY(QString  parameterFileExtension  READ parameterFileExtension CONSTANT)
    Q_PROPERTY(QString  missionFileExtension    READ missionFileExtension   CONSTANT)
    Q_PROPERTY(QString  telemetryFileExtension  READ telemetryFileExtension CONSTANT)

    /// Returns the string for distance units which has configued by user
    Q_PROPERTY(QString appSettingsDistanceUnitsString READ appSettingsDistanceUnitsString CONSTANT)
    Q_PROPERTY(QString appSettingsAreaUnitsString READ appSettingsAreaUnitsString CONSTANT)

    Q_PROPERTY(QString qgcVersion READ qgcVersion CONSTANT)

    Q_INVOKABLE void    saveGlobalSetting       (const QString& key, const QString& value);
    Q_INVOKABLE QString loadGlobalSetting       (const QString& key, const QString& defaultValue);
    Q_INVOKABLE void    saveBoolGlobalSetting   (const QString& key, bool value);
    Q_INVOKABLE bool    loadBoolGlobalSetting   (const QString& key, bool defaultValue);

    Q_INVOKABLE void    deleteAllSettingsNextBoot       () { _app->deleteAllSettingsNextBoot(); }
    Q_INVOKABLE void    clearDeleteAllSettingsNextBoot  () { _app->clearDeleteAllSettingsNextBoot(); }

    Q_INVOKABLE void    startPX4MockLink            (bool sendStatusText);
    Q_INVOKABLE void    startGenericMockLink        (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduCopterMockLink  (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduPlaneMockLink   (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduSubMockLink     (bool sendStatusText);
    Q_INVOKABLE void    stopAllMockLinks            (void);

    /// Converts from meters to the user specified distance unit
    Q_INVOKABLE QVariant metersToAppSettingsDistanceUnits(const QVariant& meters) const { return FactMetaData::metersToAppSettingsDistanceUnits(meters); }

    /// Converts from user specified distance unit to meters
    Q_INVOKABLE QVariant appSettingsDistanceUnitsToMeters(const QVariant& distance) const { return FactMetaData::appSettingsDistanceUnitsToMeters(distance); }

    QString appSettingsDistanceUnitsString(void) const { return FactMetaData::appSettingsDistanceUnitsString(); }

    /// Converts from square meters to the user specified area unit
    Q_INVOKABLE QVariant squareMetersToAppSettingsAreaUnits(const QVariant& meters) const { return FactMetaData::squareMetersToAppSettingsAreaUnits(meters); }

    /// Converts from user specified area unit to square meters
    Q_INVOKABLE QVariant appSettingsAreaUnitsToSquareMeters(const QVariant& area) const { return FactMetaData::appSettingsAreaUnitsToSquareMeters(area); }

    QString appSettingsAreaUnitsString(void) const { return FactMetaData::appSettingsAreaUnitsString(); }

    /// Returns the list of available logging category names.
    Q_INVOKABLE QStringList loggingCategories(void) const { return QGCLoggingCategoryRegister::instance()->registeredCategories(); }

    /// Turns on/off logging for the specified category. State is saved in app settings.
    Q_INVOKABLE void setCategoryLoggingOn(const QString& category, bool enable) { QGCLoggingCategoryRegister::instance()->setCategoryLoggingOn(category, enable); };

    /// Returns true if logging is turned on for the specified category.
    Q_INVOKABLE bool categoryLoggingOn(const QString& category) { return QGCLoggingCategoryRegister::instance()->categoryLoggingOn(category); };

    /// Updates the logging filter rules after settings have changed
    Q_INVOKABLE void updateLoggingFilterRules(void) { QGCLoggingCategoryRegister::instance()->setFilterRulesFromSettings(QString()); }

    Q_INVOKABLE bool linesIntersect(QPointF xLine1, QPointF yLine1, QPointF xLine2, QPointF yLine2);

    // Property accesors

    FlightMapSettings*      flightMapSettings   ()      { return _flightMapSettings; }
    LinkManager*            linkManager         ()      { return _linkManager; }
    MultiVehicleManager*    multiVehicleManager ()      { return _multiVehicleManager; }
    QGCMapEngineManager*    mapEngineManager    ()      { return _mapEngineManager; }
    QGCPositionManager*     qgcPositionManger   ()      { return _qgcPositionManager; }
    MissionCommandTree*     missionCommandTree  ()      { return _missionCommandTree; }
    VideoManager*           videoManager        ()      { return _videoManager; }
    MAVLinkLogManager*      mavlinkLogManager   ()      { return _mavlinkLogManager; }
    QGCCorePlugin*          corePlugin          ()      { return _corePlugin; }

    qreal                   zOrderTopMost       ()      { return 1000; }
    qreal                   zOrderWidgets       ()      { return 100; }
    qreal                   zOrderMapItems      ()      { return 50; }

    bool    isDarkStyle             () { return _app->styleIsDark(); }
    bool    isAudioMuted            () { return _toolbox->audioOutput()->isMuted(); }
    bool    isSaveLogPrompt         () { return _app->promptFlightDataSave(); }
    bool    isSaveLogPromptNotArmed () { return _app->promptFlightDataSaveNotArmed(); }
    bool    virtualTabletJoystick   () { return _virtualTabletJoystick; }
    qreal   baseFontPointSize       () { return _baseFontPointSize; }

    bool    isVersionCheckEnabled   () { return _toolbox->mavlinkProtocol()->versionCheckEnabled(); }
    int     mavlinkSystemID         () { return _toolbox->mavlinkProtocol()->getSystemId(); }

    QGeoCoordinate lastKnownHomePosition() { return qgcApp()->lastKnownHomePosition(); }

    static Fact* offlineEditingFirmwareType     (void);
    static Fact* offlineEditingVehicleType      (void);
    static Fact* offlineEditingCruiseSpeed      (void);
    static Fact* offlineEditingHoverSpeed       (void);
    static Fact* distanceUnits                  (void);
    static Fact* areaUnits                      (void);
    static Fact* speedUnits                     (void);
    static Fact* batteryPercentRemainingAnnounce(void);

    void    setIsDarkStyle              (bool dark);
    void    setIsAudioMuted             (bool muted);
    void    setIsSaveLogPrompt          (bool prompt);
    void    setIsSaveLogPromptNotArmed  (bool prompt);
    void    setVirtualTabletJoystick    (bool enabled);
    void    setBaseFontPointSize        (qreal size);

    void    setIsVersionCheckEnabled    (bool enable);
    void    setMavlinkSystemID          (int  id);

    QString parameterFileExtension(void) const  { return QGCApplication::parameterFileExtension; }
    QString missionFileExtension(void) const    { return QGCApplication::missionFileExtension; }
    QString telemetryFileExtension(void) const  { return QGCApplication::telemetryFileExtension; }

    QString qgcVersion(void) const { return qgcApp()->applicationVersion(); }

    // Overrides from QGCTool
    virtual void setToolbox(QGCToolbox* toolbox);

signals:
    void isDarkStyleChanged             (bool dark);
    void isAudioMutedChanged            (bool muted);
    void isSaveLogPromptChanged         (bool prompt);
    void isSaveLogPromptNotArmedChanged (bool prompt);
    void virtualTabletJoystickChanged   (bool enabled);
    void baseFontPointSizeChanged       (qreal size);
    void isMultiplexingEnabledChanged   (bool enabled);
    void isVersionCheckEnabledChanged   (bool enabled);
    void mavlinkSystemIDChanged         (int id);
    void flightMapPositionChanged       (QGeoCoordinate flightMapPosition);
    void flightMapZoomChanged           (double flightMapZoom);

private:
    static SettingsFact* _createSettingsFact(const QString& name);
    static QMap<QString, FactMetaData*>& nameToMetaDataMap(void);

    FlightMapSettings*      _flightMapSettings;
    LinkManager*            _linkManager;
    MultiVehicleManager*    _multiVehicleManager;
    QGCMapEngineManager*    _mapEngineManager;
    QGCPositionManager*     _qgcPositionManager;
    MissionCommandTree*     _missionCommandTree;
    VideoManager*           _videoManager;
    MAVLinkLogManager*      _mavlinkLogManager;
    QGCCorePlugin*          _corePlugin;

    bool                    _virtualTabletJoystick;
    qreal                   _baseFontPointSize;
    QGeoCoordinate          _flightMapPosition;
    double                  _flightMapZoom;

    // These are static so they are available to C++ code as well as Qml
    static SettingsFact*    _offlineEditingFirmwareTypeFact;
    static SettingsFact*    _offlineEditingVehicleTypeFact;
    static SettingsFact*    _offlineEditingCruiseSpeedFact;
    static SettingsFact*    _offlineEditingHoverSpeedFact;
    static SettingsFact*    _distanceUnitsFact;
    static FactMetaData*    _distanceUnitsMetaData;
    static SettingsFact*    _areaUnitsFact;
    static FactMetaData*    _areaUnitsMetaData;
    static SettingsFact*    _speedUnitsFact;
    static FactMetaData*    _speedUnitsMetaData;
    static SettingsFact*    _batteryPercentRemainingAnnounceFact;

    static const char*  _virtualTabletJoystickKey;
    static const char*  _baseFontPointSizeKey;
};

#endif
