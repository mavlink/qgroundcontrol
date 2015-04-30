/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief QGC Main Flight Display
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef QGCFLIGHTDISPLAY_H
#define QGCFLIGHTDISPLAY_H

#include "QGCQmlWidgetHolder.h"

class UASInterface;

class QGCFlightDisplay : public QGCQmlWidgetHolder
{
    Q_OBJECT
public:
    QGCFlightDisplay(QWidget* parent = NULL);
    ~QGCFlightDisplay();

    enum {
        ROLL_CHANGED,
        PITCH_CHANGED,
        HEADING_CHANGED,
        GROUNDSPEED_CHANGED,
        AIRSPEED_CHANGED,
        CLIMBRATE_CHANGED,
        ALTITUDERELATIVE_CHANGED,
        ALTITUDEWGS84_CHANGED,
        ALTITUDEAMSL_CHANGED
    };

    /// @brief Invokes the Flight Display Options menu
    void showOptionsMenu() { emit showOptionsMenuChanged(); }

    Q_PROPERTY(float roll               READ roll               NOTIFY rollChanged)
    Q_PROPERTY(float pitch              READ pitch              NOTIFY pitchChanged)
    Q_PROPERTY(float heading            READ heading            NOTIFY headingChanged)
    Q_PROPERTY(float groundSpeed        READ groundSpeed        NOTIFY groundSpeedChanged)
    Q_PROPERTY(float airSpeed           READ airSpeed           NOTIFY airSpeedChanged)
    Q_PROPERTY(float climbRate          READ climbRate          NOTIFY climbRateChanged)
    Q_PROPERTY(float altitudeRelative   READ altitudeRelative   NOTIFY altitudeRelativeChanged)
    Q_PROPERTY(float altitudeWGS84      READ altitudeWGS84      NOTIFY altitudeWGS84Changed)
    Q_PROPERTY(float altitudeAMSL       READ altitudeAMSL       NOTIFY altitudeAMSLChanged)
    Q_PROPERTY(bool  repaintRequested   READ repaintRequested   NOTIFY repaintRequestedChanged)
    Q_PROPERTY(float latitude           READ latitude           NOTIFY latitudeChanged)
    Q_PROPERTY(float longitude          READ longitude          NOTIFY longitudeChanged)
    Q_PROPERTY(bool  mavPresent         READ mavPresent         NOTIFY mavPresentChanged)

    Q_INVOKABLE void    saveSetting (const QString &key, const QString& value);
    Q_INVOKABLE QString loadSetting (const QString &key, const QString& defaultValue);

    float   roll                () { return _roll; }
    float   pitch               () { return _pitch; }
    float   heading             () { return _heading; }
    float   groundSpeed         () { return _groundSpeed; }
    float   airSpeed            () { return _airSpeed; }
    float   climbRate           () { return _climbRate; }
    float   altitudeRelative    () { return _altitudeRelative; }
    float   altitudeWGS84       () { return _altitudeWGS84; }
    float   altitudeAMSL        () { return _altitudeAMSL; }
    float   latitude            () { return _latitude; }
    float   longitude           () { return _longitude; }
    bool    repaintRequested    () { return true; }
    bool    mavPresent          () { return _mav != NULL; }

    /** @brief Start updating widget */
    void showEvent(QShowEvent* event);
    /** @brief Stop updating widget */
    void hideEvent(QHideEvent* event);

signals:
    void rollChanged            ();
    void pitchChanged           ();
    void headingChanged         ();
    void groundSpeedChanged     ();
    void airSpeedChanged        ();
    void climbRateChanged       ();
    void altitudeRelativeChanged();
    void altitudeWGS84Changed   ();
    void altitudeAMSLChanged    ();
    void repaintRequestedChanged();
    void latitudeChanged        ();
    void longitudeChanged       ();
    void mavPresentChanged      ();
    void showOptionsMenuChanged ();

private slots:
    /** @brief Attitude from main autopilot / system state */
    void _updateAttitude                    (UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp);
    /** @brief Attitude from one specific component / redundant autopilot */
    void _updateAttitude                    (UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp);
    /** @brief Speed */
    void _updateSpeed                       (UASInterface* uas, double _groundSpeed, double _airSpeed, quint64 timestamp);
    /** @brief Altitude */
    void _updateAltitude                    (UASInterface* uas, double _altitudeAMSL, double _altitudeWGS84, double _altitudeRelative, double _climbRate, quint64 timestamp);
    void _updateNavigationControllerErrors  (UASInterface* uas, double altitudeError, double speedError, double xtrackError);
    void _updateNavigationControllerData    (UASInterface *uas, float navRoll, float navPitch, float navBearing, float targetBearing, float targetDistance);
    void _forgetUAS                         (UASInterface* uas);
    void _setActiveUAS                      (UASInterface* uas);
    void _checkUpdate                       ();

private:
    bool    _isAirplane                     ();
    bool    _shouldDisplayNavigationData    ();
    void    _addChange                      (int id);
    float   _oneDecimal                     (float value);

private:
    UASInterface*   _mav;

    float _roll;
    float _pitch;
    float _heading;

    float _altitudeAMSL;
    float _altitudeWGS84;
    float _altitudeRelative;

    float _groundSpeed;
    float _airSpeed;
    float _climbRate;

    float _navigationAltitudeError;
    float _navigationSpeedError;
    float _navigationCrosstrackError;
    float _navigationTargetBearing;

    float _latitude;
    float _longitude;

    QTimer* _refreshTimer;
    QList<int> _changes;
};


#endif // QGCFLIGHTDISPLAY_H
