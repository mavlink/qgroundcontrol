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

#include <QObject>

#include "QGCApplication.h"
#include "MainWindow.h"
#include "LinkManager.h"
#include "HomePositionManager.h"
#include "FlightMapSettings.h"

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

class QGCToolbox;

class QGroundControlQmlGlobal : public QObject
{
    Q_OBJECT

public:
    QGroundControlQmlGlobal(QGCToolbox* toolbox, QObject* parent = NULL);

    Q_PROPERTY(LinkManager*         linkManager         READ linkManager            CONSTANT)
    Q_PROPERTY(MultiVehicleManager* multiVehicleManager READ multiVehicleManager    CONSTANT)
    Q_PROPERTY(HomePositionManager* homePositionManager READ homePositionManager    CONSTANT)
    Q_PROPERTY(FlightMapSettings*   flightMapSettings   READ flightMapSettings      CONSTANT)

    Q_PROPERTY(qreal                zOrderTopMost       READ zOrderTopMost          CONSTANT) ///< z order for top most items, toolbar, main window sub view
    Q_PROPERTY(qreal                zOrderWidgets       READ zOrderWidgets          CONSTANT) ///< z order value to widgets, for example: zoom controls, hud widgetss
    Q_PROPERTY(qreal                zOrderMapItems      READ zOrderMapItems         CONSTANT) ///< z order value for map items, for example: mission item indicators

    // Various QGC settings exposed to Qml
    Q_PROPERTY(bool     isDarkStyle             READ isDarkStyle                WRITE setIsDarkStyle                NOTIFY isDarkStyleChanged)              // TODO: Should be in ScreenTools?
    Q_PROPERTY(bool     isAudioMuted            READ isAudioMuted               WRITE setIsAudioMuted               NOTIFY isAudioMutedChanged)
    Q_PROPERTY(bool     isLowPowerMode          READ isLowPowerMode             WRITE setIsLowPowerMode             NOTIFY isLowPowerModeChanged)
    Q_PROPERTY(bool     isSaveLogPrompt         READ isSaveLogPrompt            WRITE setIsSaveLogPrompt            NOTIFY isSaveLogPromptChanged)
    Q_PROPERTY(bool     isSaveLogPromptNotArmed READ isSaveLogPromptNotArmed    WRITE setIsSaveLogPromptNotArmed    NOTIFY isSaveLogPromptNotArmedChanged)
    Q_PROPERTY(bool     isHeartBeatEnabled      READ isHeartBeatEnabled         WRITE setIsHeartBeatEnabled         NOTIFY isHeartBeatEnabledChanged)
    Q_PROPERTY(bool     isMultiplexingEnabled   READ isMultiplexingEnabled      WRITE setIsMultiplexingEnabled      NOTIFY isMultiplexingEnabledChanged)
    Q_PROPERTY(bool     isVersionCheckEnabled   READ isVersionCheckEnabled      WRITE setIsVersionCheckEnabled      NOTIFY isVersionCheckEnabledChanged)
    Q_PROPERTY(bool     virtualTabletJoystick   READ virtualTabletJoystick      WRITE setVirtualTabletJoystick      NOTIFY virtualTabletJoystickChanged)

    Q_INVOKABLE void    saveGlobalSetting       (const QString& key, const QString& value);
    Q_INVOKABLE QString loadGlobalSetting       (const QString& key, const QString& defaultValue);
    Q_INVOKABLE void    saveBoolGlobalSetting   (const QString& key, bool value);
    Q_INVOKABLE bool    loadBoolGlobalSetting   (const QString& key, bool defaultValue);

    Q_INVOKABLE void    deleteAllSettingsNextBoot       () { qgcApp()->deleteAllSettingsNextBoot(); }
    Q_INVOKABLE void    clearDeleteAllSettingsNextBoot  () { qgcApp()->clearDeleteAllSettingsNextBoot(); }

    Q_INVOKABLE void    startPX4MockLink            (bool sendStatusText);
    Q_INVOKABLE void    startGenericMockLink        (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduCopterMockLink  (bool sendStatusText);
    Q_INVOKABLE void    startAPMArduPlaneMockLink   (bool sendStatusText);
    Q_INVOKABLE void    stopAllMockLinks            (void);

    // Toolbar Settings
    Q_PROPERTY(bool         showGPS                 READ showGPS                WRITE  setShowGPS                   NOTIFY showGPSChanged)
    Q_PROPERTY(bool         showRCRSSI              READ showRCRSSI             WRITE  setShowRCRSSI                NOTIFY showRCRSSIChanged)
    Q_PROPERTY(bool         showTelemetryRSSI       READ showTelemetryRSSI      WRITE  SetShowTelemetryRSSI         NOTIFY showTelemetryRSSIChanged)
    Q_PROPERTY(bool         showBattery             READ showBattery            WRITE  setShowBattery               NOTIFY showBatteryChanged)
    Q_PROPERTY(bool         showBatteryConsumption  READ showBatteryConsumption WRITE  setShowBatteryConsumption    NOTIFY showBatteryConsumptionChanged)
    Q_PROPERTY(bool         showModeSelector        READ showModeSelector       WRITE  setShowModeSelector          NOTIFY showModeSelectorChanged)
    Q_PROPERTY(bool         showArmed               READ showArmed              WRITE  setShowArmed                 NOTIFY showArmedChanged)

    // Property accesors
    LinkManager*            linkManager         ()      { return _linkManager; }
    MultiVehicleManager*    multiVehicleManager ()      { return _multiVehicleManager; }
    HomePositionManager*    homePositionManager ()      { return _homePositionManager; }
    FlightMapSettings*      flightMapSettings   ()      { return _flightMapSettings; }

    qreal                   zOrderTopMost       ()      { return 1000; }
    qreal                   zOrderWidgets       ()      { return 100; }
    qreal                   zOrderMapItems      ()      { return 50; }

    bool    isDarkStyle             () { return qgcApp()->styleIsDark(); }
    bool    isAudioMuted            () { return qgcApp()->toolbox()->audioOutput()->isMuted(); }
    bool    isLowPowerMode          () { return MainWindow::instance()->lowPowerModeEnabled(); }
    bool    isSaveLogPrompt         () { return qgcApp()->promptFlightDataSave(); }
    bool    isSaveLogPromptNotArmed () { return qgcApp()->promptFlightDataSaveNotArmed(); }
    bool    isHeartBeatEnabled      () { return qgcApp()->toolbox()->mavlinkProtocol()->heartbeatsEnabled(); }
    bool    isMultiplexingEnabled   () { return qgcApp()->toolbox()->mavlinkProtocol()->multiplexingEnabled(); }
    bool    isVersionCheckEnabled   () { return qgcApp()->toolbox()->mavlinkProtocol()->versionCheckEnabled(); }
    bool    virtualTabletJoystick   () { return _virtualTabletJoystick; }

    bool    showGPS                     () { return _showGPS; }
    bool    showRCRSSI                  () { return _showRCRSSI; }
    bool    showTelemetryRSSI           () { return _showTelemRSSI; }
    bool    showBattery                 () { return _showBattery; }
    bool    showBatteryConsumption      () { return _showBatteryConsumption; }
    bool    showModeSelector            () { return _showModeSelector; }
    bool    showArmed                   () { return _showArmed; }

    //-- TODO: Make this into an actual preference.
    bool    isAdvancedMode              () { return false; }

    void    setIsDarkStyle              (bool dark);
    void    setIsAudioMuted             (bool muted);
    void    setIsLowPowerMode           (bool low);
    void    setIsSaveLogPrompt          (bool prompt);
    void    setIsSaveLogPromptNotArmed  (bool prompt);
    void    setIsHeartBeatEnabled       (bool enable);
    void    setIsMultiplexingEnabled    (bool enable);
    void    setIsVersionCheckEnabled    (bool enable);
    void    setVirtualTabletJoystick    (bool enabled);

    void    setShowGPS                  (bool state);
    void    setShowRCRSSI               (bool state);
    void    SetShowTelemetryRSSI        (bool state);
    void    setShowBattery              (bool state);
    void    setShowBatteryConsumption   (bool state);
    void    setShowModeSelector         (bool state);
    void    setShowArmed                (bool state);

signals:
    void isDarkStyleChanged             (bool dark);
    void isAudioMutedChanged            (bool muted);
    void isLowPowerModeChanged          (bool lowPower);
    void isSaveLogPromptChanged         (bool prompt);
    void isSaveLogPromptNotArmedChanged (bool prompt);
    void isHeartBeatEnabledChanged      (bool enabled);
    void isMultiplexingEnabledChanged   (bool enabled);
    void isVersionCheckEnabledChanged   (bool enabled);
    void virtualTabletJoystickChanged   (bool enabled);

    void showGPSChanged                 (bool state);
    void showRCRSSIChanged              (bool state);
    void showTelemetryRSSIChanged       (bool state);
    void showBatteryChanged             (bool state);
    void showBatteryConsumptionChanged  (bool state);
    void showModeSelectorChanged        (bool state);
    void showArmedChanged               (bool state);

private:
#ifdef QT_DEBUG
    void _startMockLink(MockConfiguration* mockConfig);
#endif

    MultiVehicleManager*    _multiVehicleManager;
    LinkManager*            _linkManager;
    HomePositionManager*    _homePositionManager;
    FlightMapSettings*      _flightMapSettings;

    bool    _virtualTabletJoystick;

    bool    _showGPS;
    bool    _showRCRSSI;
    bool    _showTelemRSSI;
    bool    _showBattery;
    bool    _showBatteryConsumption;
    bool    _showModeSelector;
    bool    _showArmed;
};

#endif
