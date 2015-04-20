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

#include "QGCFlightDisplay.h"
#include "UASManager.h"

#define UPDATE_TIMER 50

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
    , _valuesChanged(false)
    , _valuesLastPainted(0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setObjectName("MainFlightDisplay");
    // Get rid of layout default margins
    QLayout* pl = layout();
    if(pl) {
        pl->setContentsMargins(0,0,0,0);
    }
    setMinimumWidth(380);
    setMinimumHeight(360);
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
        bool update = false;
        float rolldeg = roll * (180.0 / M_PI);
        if (fabs(roll - rolldeg) > 1.0) {
            update = true;
        }
        _roll = rolldeg;
        if (update)
        {
            if(_refreshTimer->isActive()) emit rollChanged();
            _valuesChanged = true;
        }
    }
    if (isinf(pitch)) {
        _pitch = std::numeric_limits<double>::quiet_NaN();
    } else {
        bool update = false;
        float pitchdeg = pitch * (180.0 / M_PI);
        if (fabs(pitch - pitchdeg) > 1.0) {
            update = true;
        }
        _pitch = pitchdeg;
        if (update)
        {
            if(_refreshTimer->isActive()) emit pitchChanged();
            _valuesChanged = true;
        }
    }
    if (isinf(yaw)) {
        _heading = std::numeric_limits<double>::quiet_NaN();
    } else {
        bool update = false;
        yaw = yaw * (180.0 / M_PI);
        if (yaw < 0) yaw += 360;
        if (fabs(_heading - yaw) > 10.0) {
            update = true;
        }
        _heading = yaw;
        if (update)
        {
            if(_refreshTimer->isActive()) emit headingChanged();
            _valuesChanged = true;
        }
    }
}

void QGCFlightDisplay::_updateAttitude(UASInterface* uas, int, double roll, double pitch, double yaw, quint64 timestamp)
{
    _updateAttitude(uas, roll, pitch, yaw, timestamp);
}

void QGCFlightDisplay::_updateSpeed(UASInterface*, double groundSpeed, double airSpeed, quint64)
{
    double oldgroundSpeed = _groundSpeed;
    double oldairSpeed    = _airSpeed;
    _groundSpeed = groundSpeed;
    _airSpeed    = airSpeed;
    if (fabs(oldgroundSpeed - groundSpeed) > 0.5) {
        if(_refreshTimer->isActive()) emit groundSpeedChanged();
        _valuesChanged = true;
    }
    if (fabs(oldairSpeed - airSpeed) > 1.0) {
        if(_refreshTimer->isActive()) emit airSpeedChanged();
        _valuesChanged = true;
    }
}

void QGCFlightDisplay::_updateAltitude(UASInterface*, double altitudeAMSL, double altitudeWGS84, double altitudeRelative, double climbRate, quint64) {
    double oldclimbRate         = _climbRate;
    double oldaltitudeRelative  = _altitudeRelative;
    double oldaltitudeWGS84     = _altitudeWGS84;
    double oldaltitudeAMSL      = _altitudeAMSL;
    _climbRate          = climbRate;
    _altitudeRelative   = altitudeRelative;
    _altitudeWGS84      = altitudeWGS84;
    _altitudeAMSL       = altitudeAMSL;
    if(_climbRate > -0.01 && _climbRate < 0.01) {
        _climbRate = 0.0;
    }
    if (fabs(oldaltitudeAMSL - altitudeAMSL) > 0.5) {
        if(_refreshTimer->isActive()) emit altitudeAMSLChanged();
        _valuesChanged = true;
    }
    if (fabs(oldaltitudeWGS84 - altitudeWGS84) > 0.5) {
        if(_refreshTimer->isActive()) emit altitudeWGS84Changed();
        _valuesChanged = true;
    }
    if (fabs(oldaltitudeRelative - altitudeRelative) > 0.5) {
        if(_refreshTimer->isActive()) emit altitudeRelativeChanged();
        _valuesChanged = true;
    }
    if (fabs(oldclimbRate - climbRate) > 0.5) {
        if(_refreshTimer->isActive()) emit climbRateChanged();
        _valuesChanged = true;
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
    // React only to internal (pre-display) events
    QWidget::showEvent(event);
    _refreshTimer->start(UPDATE_TIMER);
}

void QGCFlightDisplay::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display) events
    _refreshTimer->stop();
    QWidget::hideEvent(event);
}

void QGCFlightDisplay::_checkUpdate()
{
    if (_mav && (_valuesChanged || (QGC::groundTimeMilliseconds() - _valuesLastPainted) > 260)) {
        _valuesChanged = false;
        _valuesLastPainted = QGC::groundTimeMilliseconds();
        emit rollChanged();
        emit pitchChanged();
        emit headingChanged();
        emit altitudeAMSLChanged();
        emit altitudeWGS84Changed();
        emit altitudeRelativeChanged();
        emit climbRateChanged();
        emit groundSpeedChanged();
        emit airSpeedChanged();
        emit repaintRequestedChanged();
        emit latitudeChanged();
        emit longitudeChanged();
    }
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
}

