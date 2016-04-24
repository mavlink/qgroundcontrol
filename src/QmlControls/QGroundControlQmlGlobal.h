/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef QGroundControlQmlGlobal_H
#define QGroundControlQmlGlobal_H

#include "QGCToolbox.h"
#include "QGCApplication.h"
#include "LinkManager.h"
#include "HomePositionManager.h"
#include "FlightMapSettings.h"
#include "MissionCommands.h"
#include "SettingsFact.h"
#include "FactMetaData.h"
#include "SimulatedPosition.h"

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

    enum SpeedUnits {
        SpeedUnitsFeetPerSecond = 0,
        SpeedUnitsMetersPerSecond,
        SpeedUnitsMilesPerHour,
        SpeedUnitsKilometersPerHour,
        SpeedUnitsKnots,
    };

    Q_ENUMS(DistanceUnits)
    Q_ENUMS(SpeedUnits)

    Q_PROPERTY(FlightMapSettings*   flightMapSettings   READ flightMapSettings      CONSTANT)
    Q_PROPERTY(HomePositionManager* homePositionManager READ homePositionManager    CONSTANT)
    Q_PROPERTY(LinkManager*         linkManager         READ linkManager            CONSTANT)
    Q_PROPERTY(MissionCommands*     missionCommands     READ missionCommands        CONSTANT)
    Q_PROPERTY(MultiVehicleManager* multiVehicleManager READ multiVehicleManager    CONSTANT)
    Q_PROPERTY(QGCMapEngineManager* mapEngineManager    READ mapEngineManager       CONSTANT)
    Q_PROPERTY(QGCPositionManager*  qgcPositionManger   READ qgcPositionManger      CONSTANT)

    Q_PROPERTY(qreal                zOrderTopMost       READ zOrderTopMost          CONSTANT) ///< z order for top most items, toolbar, main window sub view
    Q_PROPERTY(qreal                zOrderWidgets       READ zOrderWidgets          CONSTANT) ///< z order value to widgets, for example: zoom controls, hud widgetss
    Q_PROPERTY(qreal                zOrderMapItems      READ zOrderMapItems         CONSTANT) ///< z order value for map items, for example: mission item indicators

    // Various QGC settings exposed to Qml
    Q_PROPERTY(bool     isAdvancedMode          READ isAdvancedMode                                                 CONSTANT)                               ///< Global "Advance Mode" preference. Certain UI elements and features are different based on this.
    Q_PROPERTY(bool     isDarkStyle             READ isDarkStyle                WRITE setIsDarkStyle                NOTIFY isDarkStyleChanged)              // TODO: Should be in ScreenTools?
    Q_PROPERTY(bool     isAudioMuted            READ isAudioMuted               WRITE setIsAudioMuted               NOTIFY isAudioMutedChanged)
    Q_PROPERTY(bool     isSaveLogPrompt         READ isSaveLogPrompt            WRITE setIsSaveLogPrompt            NOTIFY isSaveLogPromptChanged)
    Q_PROPERTY(bool     isSaveLogPromptNotArmed READ isSaveLogPromptNotArmed    WRITE setIsSaveLogPromptNotArmed    NOTIFY isSaveLogPromptNotArmedChanged)
    Q_PROPERTY(bool     virtualTabletJoystick   READ virtualTabletJoystick      WRITE setVirtualTabletJoystick      NOTIFY virtualTabletJoystickChanged)

    // MavLink Protocol
    Q_PROPERTY(bool     isMultiplexingEnabled   READ isMultiplexingEnabled      WRITE setIsMultiplexingEnabled      NOTIFY isMultiplexingEnabledChanged)
    Q_PROPERTY(bool     isVersionCheckEnabled   READ isVersionCheckEnabled      WRITE setIsVersionCheckEnabled      NOTIFY isVersionCheckEnabledChanged)
    Q_PROPERTY(int      mavlinkSystemID         READ mavlinkSystemID            WRITE setMavlinkSystemID            NOTIFY mavlinkSystemIDChanged)

    Q_PROPERTY(Fact*    offlineEditingFirmwareType  READ offlineEditingFirmwareType CONSTANT)
    Q_PROPERTY(Fact*    distanceUnits               READ distanceUnits              CONSTANT)
    Q_PROPERTY(Fact*    speedUnits                  READ speedUnits                 CONSTANT)

    Q_PROPERTY(QGeoCoordinate lastKnownHomePosition READ lastKnownHomePosition  CONSTANT)
    Q_PROPERTY(QGeoCoordinate flightMapPosition     MEMBER _flightMapPosition   NOTIFY flightMapPositionChanged)
    Q_PROPERTY(double         flightMapZoom         MEMBER _flightMapZoom       NOTIFY flightMapZoomChanged)

    Q_PROPERTY(QString  parameterFileExtension  READ parameterFileExtension CONSTANT)
    Q_PROPERTY(QString  missionFileExtension    READ missionFileExtension   CONSTANT)
    Q_PROPERTY(QString  telemetryFileExtension  READ telemetryFileExtension CONSTANT)

    /// @ return: true: experimental survey ip code is turned on
    Q_PROPERTY(bool experimentalSurvey READ experimentalSurvey WRITE setExperimentalSurvey NOTIFY experimentalSurveyChanged)

    /// Returns the string for distance units which has configued by user
    Q_PROPERTY(QString appSettingsDistanceUnitsString READ appSettingsDistanceUnitsString CONSTANT)

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
    Q_INVOKABLE void    stopAllMockLinks            (void);

    /// Converts from meters to the user specified distance unit
    Q_INVOKABLE QVariant metersToAppSettingsDistanceUnits(const QVariant& meters) const { return FactMetaData::metersToAppSettingsDistanceUnits(meters); }

    /// Converts from user specified distance unit to meters
    Q_INVOKABLE QVariant appSettingsDistanceUnitsToMeters(const QVariant& distance) const { return FactMetaData::appSettingsDistanceUnitsToMeters(distance); }

    QString appSettingsDistanceUnitsString(void) const { return FactMetaData::appSettingsDistanceUnitsString(); }

    // Property accesors

    FlightMapSettings*      flightMapSettings   ()      { return _flightMapSettings; }
    HomePositionManager*    homePositionManager ()      { return _homePositionManager; }
    LinkManager*            linkManager         ()      { return _linkManager; }
    MissionCommands*        missionCommands     ()      { return _missionCommands; }
    MultiVehicleManager*    multiVehicleManager ()      { return _multiVehicleManager; }
    QGCMapEngineManager*    mapEngineManager    ()      { return _mapEngineManager; }
    QGCPositionManager*     qgcPositionManger   ()      { return _qgcPositionManager; }

    qreal                   zOrderTopMost       ()      { return 1000; }
    qreal                   zOrderWidgets       ()      { return 100; }
    qreal                   zOrderMapItems      ()      { return 50; }

    bool    isDarkStyle             () { return _app->styleIsDark(); }
    bool    isAudioMuted            () { return _toolbox->audioOutput()->isMuted(); }
    bool    isSaveLogPrompt         () { return _app->promptFlightDataSave(); }
    bool    isSaveLogPromptNotArmed () { return _app->promptFlightDataSaveNotArmed(); }
    bool    virtualTabletJoystick   () { return _virtualTabletJoystick; }

    bool    isMultiplexingEnabled   () { return _toolbox->mavlinkProtocol()->multiplexingEnabled(); }
    bool    isVersionCheckEnabled   () { return _toolbox->mavlinkProtocol()->versionCheckEnabled(); }
    int     mavlinkSystemID         () { return _toolbox->mavlinkProtocol()->getSystemId(); }

    QGeoCoordinate lastKnownHomePosition() { return qgcApp()->lastKnownHomePosition(); }

    static Fact* offlineEditingFirmwareType (void);
    static Fact* distanceUnits              (void);
    static Fact* speedUnits                 (void);

    //-- TODO: Make this into an actual preference.
    bool    isAdvancedMode          () { return false; }

    void    setIsDarkStyle              (bool dark);
    void    setIsAudioMuted             (bool muted);
    void    setIsSaveLogPrompt          (bool prompt);
    void    setIsSaveLogPromptNotArmed  (bool prompt);
    void    setVirtualTabletJoystick    (bool enabled);

    void    setIsMultiplexingEnabled    (bool enable);
    void    setIsVersionCheckEnabled    (bool enable);
    void    setMavlinkSystemID          (int  id);

    bool experimentalSurvey(void) const;
    void setExperimentalSurvey(bool experimentalSurvey);

    QString parameterFileExtension(void) const  { return QGCApplication::parameterFileExtension; }
    QString missionFileExtension(void) const    { return QGCApplication::missionFileExtension; }
    QString telemetryFileExtension(void) const  { return QGCApplication::telemetryFileExtension; }

    // Overrides from QGCTool
    virtual void setToolbox(QGCToolbox* toolbox);

signals:
    void isDarkStyleChanged             (bool dark);
    void isAudioMutedChanged            (bool muted);
    void isSaveLogPromptChanged         (bool prompt);
    void isSaveLogPromptNotArmedChanged (bool prompt);
    void virtualTabletJoystickChanged   (bool enabled);
    void isMultiplexingEnabledChanged   (bool enabled);
    void isVersionCheckEnabledChanged   (bool enabled);
    void mavlinkSystemIDChanged         (int id);
    void flightMapPositionChanged       (QGeoCoordinate flightMapPosition);
    void flightMapZoomChanged           (double flightMapZoom);
    void experimentalSurveyChanged      (bool experimentalSurvey);

private:
    FlightMapSettings*      _flightMapSettings;
    HomePositionManager*    _homePositionManager;
    LinkManager*            _linkManager;
    MissionCommands*        _missionCommands;
    MultiVehicleManager*    _multiVehicleManager;
    QGCMapEngineManager*    _mapEngineManager;
    QGCPositionManager*     _qgcPositionManager;

    bool _virtualTabletJoystick;

    QGeoCoordinate  _flightMapPosition;
    double          _flightMapZoom;

    // These are static so they are available to C++ code as well as Qml
    static SettingsFact*    _offlineEditingFirmwareTypeFact;
    static FactMetaData*    _offlineEditingFirmwareTypeMetaData;
    static SettingsFact*    _distanceUnitsFact;
    static FactMetaData*    _distanceUnitsMetaData;
    static SettingsFact*    _speedUnitsFact;
    static FactMetaData*    _speedUnitsMetaData;

    static const char*  _virtualTabletJoystickKey;
};

#endif
