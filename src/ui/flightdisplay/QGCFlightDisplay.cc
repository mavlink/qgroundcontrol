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

#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>

#include "MainWindow.h"
#include "QGCFlightDisplay.h"
#include "UASManager.h"

#define UPDATE_TIMER 100

const char* kMainFlightDisplayGroup = "MainFlightDisplay";

QGCFlightDisplay::QGCFlightDisplay(QWidget *parent)
    : QGCQmlWidgetHolder(parent)
    , _mav(NULL)
    , _roll(0.0f)
    , _pitch(0.0f)
    , _heading(0.0f)
    , _altitudeAMSL(0.0f)
    , _altitudeWGS84(0.0f)
    , _altitudeRelative(0.0f)
    , _groundSpeed(0.0f)
    , _airSpeed(0.0f)
    , _climbRate(0.0f)
    , _navigationAltitudeError(0.0f)
    , _navigationSpeedError(0.0f)
    , _navigationCrosstrackError(0.0f)
    , _navigationTargetBearing(0.0f)
    , _latitude(37.803784f)
    , _longitude(-122.462276f)
    , _refreshTimer(new QTimer(this))
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setObjectName("MainFlightDisplay");
    // Get rid of layout default margins
    QLayout* pl = layout();
    if(pl) {
        pl->setContentsMargins(0,0,0,0);
    }
#ifndef __android__
    setMinimumWidth( 380 * MainWindow::pixelSizeFactor());
    setMinimumHeight(400 * MainWindow::pixelSizeFactor());
#endif
    setContextPropertyObject("flightDisplay", this);
    setSource(QUrl::fromUserInput("qrc:/qml/FlightDisplay.qml"));
    setVisible(true);
    // Connect with UAS signal
    _setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(UASDeleted(UASInterface*)),   this, SLOT(_forgetUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(_setActiveUAS(UASInterface*)));
    // Refresh timer
    _refreshTimer->setInterval(UPDATE_TIMER);
    connect(_refreshTimer, SIGNAL(timeout()), this, SLOT(_checkUpdate()));
}

QGCFlightDisplay::~QGCFlightDisplay()
{
    _refreshTimer->stop();
}

void QGCFlightDisplay::saveSetting(const QString &name, const QString& value)
{
    QSettings settings;
    QString key(kMainFlightDisplayGroup);
    key += "/" + name;
    settings.setValue(key, value);
}

QString QGCFlightDisplay::loadSetting(const QString &name, const QString& defaultValue)
{
    QSettings settings;
    QString key(kMainFlightDisplayGroup);
    key += "/" + name;
    return settings.value(key, defaultValue).toString();
}

void QGCFlightDisplay::_forgetUAS(UASInterface* uas)
{
    if (_mav != NULL && _mav == uas) {
        // Disconnect any previously connected active MAV
        disconnect(_mav, SIGNAL(attitudeChanged                  (UASInterface*, double,double,double,quint64)),            this, SLOT(_updateAttitude(UASInterface*, double, double, double, quint64)));
        disconnect(_mav, SIGNAL(attitudeChanged                  (UASInterface*, int,double,double,double,quint64)),        this, SLOT(_updateAttitude(UASInterface*,int,double, double, double, quint64)));
        disconnect(_mav, SIGNAL(speedChanged                     (UASInterface*, double, double, quint64)),                 this, SLOT(_updateSpeed(UASInterface*, double, double, quint64)));
        disconnect(_mav, SIGNAL(altitudeChanged                  (UASInterface*, double, double, double, double, quint64)), this, SLOT(_updateAltitude(UASInterface*, double, double, double, double, quint64)));
        disconnect(_mav, SIGNAL(navigationControllerErrorsChanged(UASInterface*, double, double, double)),                  this, SLOT(_updateNavigationControllerErrors(UASInterface*, double, double, double)));
        disconnect(_mav, &UASInterface::NavigationControllerDataChanged, this, &QGCFlightDisplay::_updateNavigationControllerData);
    }
    _mav = NULL;
    emit mavPresentChanged();
}

void QGCFlightDisplay::_setActiveUAS(UASInterface* uas)
{
    if (uas == _mav)
        return;
    // Disconnect the previous one (if any)
    if(_mav) {
        _forgetUAS(_mav);
    }
    if (uas) {
        // Now connect the new UAS
        // Setup communication
        connect(uas, SIGNAL(attitudeChanged                     (UASInterface*,double,double,double,quint64)),              this, SLOT(_updateAttitude(UASInterface*, double, double, double, quint64)));
        connect(uas, SIGNAL(attitudeChanged                     (UASInterface*,int,double,double,double,quint64)),          this, SLOT(_updateAttitude(UASInterface*,int,double, double, double, quint64)));
        connect(uas, SIGNAL(speedChanged                        (UASInterface*, double, double, quint64)),                  this, SLOT(_updateSpeed(UASInterface*, double, double, quint64)));
        connect(uas, SIGNAL(altitudeChanged                     (UASInterface*, double, double, double, double, quint64)),  this, SLOT(_updateAltitude(UASInterface*, double, double, double, double, quint64)));
        connect(uas, SIGNAL(navigationControllerErrorsChanged   (UASInterface*, double, double, double)),                   this, SLOT(_updateNavigationControllerErrors(UASInterface*, double, double, double)));
        connect(uas, &UASInterface::NavigationControllerDataChanged, this, &QGCFlightDisplay::_updateNavigationControllerData);
        // Set new UAS
        _mav = uas;
    }
    emit mavPresentChanged();
}

void QGCFlightDisplay::_updateAttitude(UASInterface*, double roll, double pitch, double yaw, quint64)
{
    if (isinf(roll)) {
        _roll = std::numeric_limits<double>::quiet_NaN();
    } else {
        float rolldeg = _oneDecimal(roll * (180.0 / M_PI));
        if (fabs(roll - rolldeg) > 1.0) {
            _roll = rolldeg;
            if(_refreshTimer->isActive()) {
                emit rollChanged();
            }
        }
        if(_roll != rolldeg) {
            _roll = rolldeg;
            _addChange(ROLL_CHANGED);
        }
    }
    if (isinf(pitch)) {
        _pitch = std::numeric_limits<double>::quiet_NaN();
    } else {
        float pitchdeg = _oneDecimal(pitch * (180.0 / M_PI));
        if (fabs(pitch - pitchdeg) > 1.0) {
            _pitch = pitchdeg;
            if(_refreshTimer->isActive()) {
                emit pitchChanged();
            }
        }
        if(_pitch != pitchdeg) {
            _pitch = pitchdeg;
            _addChange(PITCH_CHANGED);
        }
    }
    if (isinf(yaw)) {
        _heading = std::numeric_limits<double>::quiet_NaN();
    } else {
        yaw = _oneDecimal(yaw * (180.0 / M_PI));
        if (yaw < 0) yaw += 360;
        if (fabs(_heading - yaw) > 1.0) {
            _heading = yaw;
            if(_refreshTimer->isActive()) {
                emit headingChanged();
            }
        }
        if(_heading != yaw) {
            _heading = yaw;
            _addChange(HEADING_CHANGED);
        }
    }
}

void QGCFlightDisplay::_updateAttitude(UASInterface* uas, int, double roll, double pitch, double yaw, quint64 timestamp)
{
    _updateAttitude(uas, roll, pitch, yaw, timestamp);
}

void QGCFlightDisplay::_updateSpeed(UASInterface*, double groundSpeed, double airSpeed, quint64)
{
    groundSpeed = _oneDecimal(groundSpeed);
    if (fabs(_groundSpeed - groundSpeed) > 0.5) {
        _groundSpeed = groundSpeed;
        if(_refreshTimer->isActive()) {
            emit groundSpeedChanged();
        }
    }
    airSpeed = _oneDecimal(airSpeed);
    if (fabs(_airSpeed - airSpeed) > 1.0) {
        _airSpeed = airSpeed;
        if(_refreshTimer->isActive()) {
            emit airSpeedChanged();
        }
    }
    if(_groundSpeed != groundSpeed) {
        _groundSpeed = groundSpeed;
        _addChange(GROUNDSPEED_CHANGED);
    }
    if(_airSpeed != airSpeed) {
        _airSpeed = airSpeed;
        _addChange(AIRSPEED_CHANGED);
    }
}

void QGCFlightDisplay::_updateAltitude(UASInterface*, double altitudeAMSL, double altitudeWGS84, double altitudeRelative, double climbRate, quint64) {
    altitudeAMSL = _oneDecimal(altitudeAMSL);
    if (fabs(_altitudeAMSL - altitudeAMSL) > 0.5) {
        _altitudeAMSL = altitudeAMSL;
        if(_refreshTimer->isActive()) {
            emit altitudeAMSLChanged();
        }
    }
    altitudeWGS84 = _oneDecimal(altitudeWGS84);
    if (fabs(_altitudeWGS84 - altitudeWGS84) > 0.5) {
        _altitudeWGS84 = altitudeWGS84;
        if(_refreshTimer->isActive()) {
            emit altitudeWGS84Changed();
        }
    }
    altitudeRelative = _oneDecimal(altitudeRelative);
    if (fabs(_altitudeRelative - altitudeRelative) > 0.5) {
        _altitudeRelative = altitudeRelative;
        if(_refreshTimer->isActive()) {
            emit altitudeRelativeChanged();
        }
    }
    climbRate = _oneDecimal(climbRate);
    if (fabs(_climbRate - climbRate) > 0.5) {
        _climbRate = climbRate;
        if(_refreshTimer->isActive()) {
            emit climbRateChanged();
        }
    }
    if(_altitudeAMSL != altitudeAMSL) {
        _altitudeAMSL = altitudeAMSL;
        _addChange(ALTITUDEAMSL_CHANGED);
    }
    if(_altitudeWGS84 != altitudeWGS84) {
        _altitudeWGS84 = altitudeWGS84;
        _addChange(ALTITUDEWGS84_CHANGED);
    }
    if(_altitudeRelative != altitudeRelative) {
        _altitudeRelative = altitudeRelative;
        _addChange(ALTITUDERELATIVE_CHANGED);
    }
    if(_climbRate != climbRate) {
        _climbRate = climbRate;
        _addChange(CLIMBRATE_CHANGED);
    }
}

void QGCFlightDisplay::_updateNavigationControllerErrors(UASInterface*, double altitudeError, double speedError, double xtrackError) {
    _navigationAltitudeError   = altitudeError;
    _navigationSpeedError      = speedError;
    _navigationCrosstrackError = xtrackError;
}

void QGCFlightDisplay::_updateNavigationControllerData(UASInterface *uas, float, float, float, float targetBearing, float) {
    if (_mav == uas) {
        _navigationTargetBearing = targetBearing;
    }
}

/*
 * Internal
 */

bool QGCFlightDisplay::_isAirplane() {
    if (_mav)
        return _mav->isAirplane();
    return false;
}

// TODO: Implement. Should return true when navigating.
// That would be (PX4) in AUTO and RTL modes.
// This could forward to a virtual on UAS bool isNavigatingAutonomusly() or whatever.
bool QGCFlightDisplay::_shouldDisplayNavigationData() {
    return true;
}

void QGCFlightDisplay::showEvent(QShowEvent* event)
{
    // Enable updates
    QWidget::showEvent(event);
    _refreshTimer->start(UPDATE_TIMER);
}

void QGCFlightDisplay::hideEvent(QHideEvent* event)
{
    // Disable updates
    _refreshTimer->stop();
    QWidget::hideEvent(event);
}

void QGCFlightDisplay::_addChange(int id)
{
    if(!_changes.contains(id)) {
        _changes.append(id);
    }
}

float QGCFlightDisplay::_oneDecimal(float value)
{
    int i = (value * 10);
    return (float)i / 10.0;
}

void QGCFlightDisplay::_checkUpdate()
{
    // Update current location
    if(_mav) {
        if(_latitude != _mav->getLatitude()) {
            _latitude = _mav->getLatitude();
            emit latitudeChanged();
        }
        if(_longitude != _mav->getLongitude()) {
            _longitude = _mav->getLongitude();
            emit longitudeChanged();
        }
    }
    // Check for changes
    // Significant changes, that is, whole number changes, are updated immediatelly.
    // For every message however, we set a flag for what changed and this timer updates
    // them to bring everything up-to-date. This prevents an avalanche of UI updates.
    foreach(int i, _changes) {
        switch (i) {
        case ROLL_CHANGED:
            emit rollChanged();
            break;
        case PITCH_CHANGED:
            emit pitchChanged();
            break;
        case HEADING_CHANGED:
            emit headingChanged();
            break;
        case GROUNDSPEED_CHANGED:
            emit groundSpeedChanged();
            break;
        case AIRSPEED_CHANGED:
            emit airSpeedChanged();
            break;
        case CLIMBRATE_CHANGED:
            emit climbRateChanged();
            break;
        case ALTITUDERELATIVE_CHANGED:
            emit altitudeRelativeChanged();
            break;
        case ALTITUDEWGS84_CHANGED:
            emit altitudeWGS84Changed();
            break;
        case ALTITUDEAMSL_CHANGED:
            emit altitudeAMSLChanged();
            break;
        default:
            break;
        }
    }
    _changes.clear();
}

