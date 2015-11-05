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
#include "HomePositionManager.h"
#include "FlightMapSettings.h"

class QGCToolbox;

class QGroundControlQmlGlobal : public QObject
{
    Q_OBJECT

public:
    QGroundControlQmlGlobal(QGCToolbox* toolbox, QObject* parent = NULL);

    Q_PROPERTY(HomePositionManager* homePositionManager READ homePositionManager    CONSTANT)
    Q_PROPERTY(FlightMapSettings*   flightMapSettings   READ flightMapSettings      CONSTANT)

    Q_PROPERTY(qreal                zOrderTopMost       READ zOrderTopMost          CONSTANT) ///< z order for top most items, toolbar, main window sub view
    Q_PROPERTY(qreal                zOrderWidgets       READ zOrderWidgets          CONSTANT) ///< z order value to widgets, for example: zoom controls, hud widgetss
    Q_PROPERTY(qreal                zOrderMapItems      READ zOrderMapItems         CONSTANT) ///< z order value for map items, for example: mission item indicators

    /// Global "Advance Mode" preference. Certain UI elements and features are different based on this.
    Q_PROPERTY(bool                 isAdvancedMode      READ isAdvancedMode         CONSTANT)

    Q_INVOKABLE void    saveGlobalSetting       (const QString& key, const QString& value);
    Q_INVOKABLE QString loadGlobalSetting       (const QString& key, const QString& defaultValue);
    Q_INVOKABLE void    saveBoolGlobalSetting   (const QString& key, bool value);
    Q_INVOKABLE bool    loadBoolGlobalSetting   (const QString& key, bool defaultValue);

    // Property accesors

    HomePositionManager*    homePositionManager ()      { return _homePositionManager; }
    FlightMapSettings*      flightMapSettings   ()      { return _flightMapSettings; }

    qreal                   zOrderTopMost       ()      { return 1000; }
    qreal                   zOrderWidgets       ()      { return 100; }
    qreal                   zOrderMapItems      ()      { return 50; }

    //-- TODO: This should be in ScreenTools but I don't understand the changes done there (ScreenToolsController versus ScreenTools)
    Q_PROPERTY(bool     isDarkStyle         READ isDarkStyle     WRITE setIsDarkStyle    NOTIFY isDarkStyleChanged)
    bool    isDarkStyle         ()              { return qgcApp()->styleIsDark(); }
    void    setIsDarkStyle      (bool dark)     { qgcApp()->setStyle(dark); }

    //-- Audio Muting
    Q_PROPERTY(bool     isAudioMuted        READ isAudioMuted    WRITE setIsAudioMuted    NOTIFY isAudioMutedChanged)
    bool    isAudioMuted        ()              { return qgcApp()->toolbox()->audioOutput()->isMuted(); }
    void    setIsAudioMuted     (bool muted)    { qgcApp()->toolbox()->audioOutput()->mute(muted); }

    //-- Low power mode
    Q_PROPERTY(bool     isLowPowerMode      READ isLowPowerMode  WRITE setIsLowPowerMode   NOTIFY isLowPowerModeChanged)
    bool    isLowPowerMode      ()              { return MainWindow::instance()->lowPowerModeEnabled(); }
    void    setIsLowPowerMode   (bool low)      { MainWindow::instance()->enableLowPowerMode(low); }

    //-- Prompt save log
    Q_PROPERTY(bool     isSaveLogPrompt     READ isSaveLogPrompt WRITE setIsSaveLogPrompt  NOTIFY isSaveLogPromptChanged)
    bool    isSaveLogPrompt     ()              { return qgcApp()->promptFlightDataSave(); }
    void    setIsSaveLogPrompt  (bool prompt)   { qgcApp()->setPromptFlightDataSave(prompt); }

    //-- ClearSettings
    Q_INVOKABLE void    deleteAllSettingsNextBoot       () { qgcApp()->deleteAllSettingsNextBoot(); }
    Q_INVOKABLE void    clearDeleteAllSettingsNextBoot  () { qgcApp()->clearDeleteAllSettingsNextBoot(); }

    //-- TODO: Make this into an actual preference.
    bool                    isAdvancedMode      ()      { return false; }

    //
    //-- Mavlink Protocol
    //

    //-- Emit heartbeat
    Q_PROPERTY(bool     isHeartBeatEnabled  READ isHeartBeatEnabled  WRITE setIsHeartBeatEnabled  NOTIFY isHeartBeatEnabledChanged)
    bool    isHeartBeatEnabled     ()              { return qgcApp()->toolbox()->mavlinkProtocol()->heartbeatsEnabled(); }
    void    setIsHeartBeatEnabled  (bool enable)   { qgcApp()->toolbox()->mavlinkProtocol()->enableHeartbeats(enable); }

    //-- Multiplexing
    Q_PROPERTY(bool     isMultiplexingEnabled  READ isMultiplexingEnabled  WRITE setIsMultiplexingEnabled  NOTIFY isMultiplexingEnabledChanged)
    bool    isMultiplexingEnabled   ()              { return qgcApp()->toolbox()->mavlinkProtocol()->multiplexingEnabled(); }
    void    setIsMultiplexingEnabled(bool enable)   { qgcApp()->toolbox()->mavlinkProtocol()->enableMultiplexing(enable); }

    //-- Version Check
    Q_PROPERTY(bool     isVersionCheckEnabled  READ isVersionCheckEnabled  WRITE setIsVersionCheckEnabled  NOTIFY isVersionCheckEnabledChanged)
    bool    isVersionCheckEnabled   ()              { return qgcApp()->toolbox()->mavlinkProtocol()->versionCheckEnabled(); }
    void    setIsVersionCheckEnabled(bool enable)   { qgcApp()->toolbox()->mavlinkProtocol()->enableVersionCheck(enable); }

signals:
    void isDarkStyleChanged             (bool dark);
    void isAudioMutedChanged            (bool muted);
    void isLowPowerModeChanged          (bool lowPower);
    void isSaveLogPromptChanged         (bool prompt);
    void isHeartBeatEnabledChanged      (bool enabled);
    void isMultiplexingEnabledChanged   (bool enabled);
    void isVersionCheckEnabledChanged   (bool enabled);

private:
    HomePositionManager*    _homePositionManager;
    FlightMapSettings*      _flightMapSettings;
};

#endif
