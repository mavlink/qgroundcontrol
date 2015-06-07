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
 *   @brief QGC Mav Manager (QML Bindings)
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "UAS.h"
#include "MainWindow.h"
#include "UASManager.h"
#include "Waypoint.h"
#include "MavManager.h"
#include "UASMessageHandler.h"

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f

MavManager::MavManager(QObject *parent)
    : QObject(parent)
    , _mav(NULL)
    , _currentMessageCount(0)
    , _messageCount(0)
    , _currentErrorCount(0)
    , _currentWarningCount(0)
    , _currentNormalCount(0)
    , _currentMessageType(MessageNone)
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
    , _latitude(DEFAULT_LAT)
    , _longitude(DEFAULT_LON)
    , _refreshTimer(new QTimer(this))
    , _batteryVoltage(0.0)
    , _batteryPercent(0.0)
    , _batteryConsumed(-1.0)
    , _systemArmed(false)
    , _currentHeartbeatTimeout(0)
    , _waypointDistance(0.0)
    , _currentWaypoint(0)
    , _satelliteCount(-1)
    , _satelliteLock(0)
    , _wpm(NULL)
    , _updateCount(0)
{
    // Connect with UAS signal
    _setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(UASDeleted(UASInterface*)),   this, SLOT(_forgetUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(_setActiveUAS(UASInterface*)));
    // Refresh timer
    connect(_refreshTimer, SIGNAL(timeout()), this, SLOT(_checkUpdate()));
    _refreshTimer->setInterval(UPDATE_TIMER);
    _refreshTimer->start(UPDATE_TIMER);
    emit heartbeatTimeoutChanged();
}

MavManager::~MavManager()
{
    _refreshTimer->stop();
}

void MavManager::saveSetting(const QString &name, const QString& value)
{
    QSettings settings;
    settings.setValue(name, value);
}

QString MavManager::loadSetting(const QString &name, const QString& defaultValue)
{
    QSettings settings;
    return settings.value(name, defaultValue).toString();
}

void MavManager::_forgetUAS(UASInterface* uas)
{
    if (_mav != NULL && _mav == uas) {
        // Stop listening for system messages
        disconnect(UASMessageHandler::instance(), &UASMessageHandler::textMessageCountChanged,  this, &MavManager::_handleTextMessage);
        // Disconnect any previously connected active MAV
        disconnect(_mav, SIGNAL(attitudeChanged                     (UASInterface*, double,double,double,quint64)),             this, SLOT(_updateAttitude(UASInterface*, double, double, double, quint64)));
        disconnect(_mav, SIGNAL(attitudeChanged                     (UASInterface*, int,double,double,double,quint64)),         this, SLOT(_updateAttitude(UASInterface*,int,double, double, double, quint64)));
        disconnect(_mav, SIGNAL(speedChanged                        (UASInterface*, double, double, quint64)),                  this, SLOT(_updateSpeed(UASInterface*, double, double, quint64)));
        disconnect(_mav, SIGNAL(altitudeChanged                     (UASInterface*, double, double, double, double, quint64)),  this, SLOT(_updateAltitude(UASInterface*, double, double, double, double, quint64)));
        disconnect(_mav, SIGNAL(navigationControllerErrorsChanged   (UASInterface*, double, double, double)),                   this, SLOT(_updateNavigationControllerErrors(UASInterface*, double, double, double)));
        disconnect(_mav, SIGNAL(statusChanged                       (UASInterface*,QString,QString)),                           this, SLOT(_updateState(UASInterface*,QString,QString)));
        disconnect(_mav, SIGNAL(armingChanged                       (bool)),                                                    this, SLOT(_updateArmingState(bool)));
        disconnect(_mav, &UASInterface::NavigationControllerDataChanged, this, &MavManager::_updateNavigationControllerData);
        disconnect(_mav, &UASInterface::heartbeatTimeout,                this, &MavManager::_heartbeatTimeout);
        disconnect(_mav, &UASInterface::batteryChanged,                  this, &MavManager::_updateBatteryRemaining);
        disconnect(_mav, &UASInterface::batteryConsumedChanged,          this, &MavManager::_updateBatteryConsumedChanged);
        disconnect(_mav, &UASInterface::modeChanged,                     this, &MavManager::_updateMode);
        disconnect(_mav, &UASInterface::nameChanged,                     this, &MavManager::_updateName);
        disconnect(_mav, &UASInterface::systemTypeSet,                   this, &MavManager::_setSystemType);
        disconnect(_mav, &UASInterface::localizationChanged,             this, &MavManager::_setSatLoc);
        if (_wpm) {
            disconnect(_wpm, &UASWaypointManager::currentWaypointChanged,    this, &MavManager::_updateCurrentWaypoint);
            disconnect(_wpm, &UASWaypointManager::waypointDistanceChanged,   this, &MavManager::_updateWaypointDistance);
            disconnect(_wpm, SIGNAL(waypointViewOnlyListChanged(void)),      this, SLOT(_waypointViewOnlyListChanged(void)));
            disconnect(_wpm, SIGNAL(waypointViewOnlyChanged(int,Waypoint*)), this, SLOT(_updateWaypointViewOnly(int,Waypoint*)));
        }
        UAS* pUas = dynamic_cast<UAS*>(_mav);
        if(pUas) {
            disconnect(pUas, &UAS::satelliteCountChanged, this, &MavManager::_setSatelliteCount);
        }
        _mav = NULL;
        _satelliteCount = -1;
        emit mavPresentChanged();
        emit satelliteCountChanged();
    }
}

void MavManager::_setActiveUAS(UASInterface* uas)
{
    if (uas == _mav)
        return;
    // Disconnect the previous one (if any)
    if(_mav) {
        _forgetUAS(_mav);
    }
    if (uas) {
        // Reset satellite count (no GPS)
        _satelliteCount = -1;
        emit satelliteCountChanged();
        // Reset connection lost (if any)
        _currentHeartbeatTimeout = 0;
        emit heartbeatTimeoutChanged();
        // Set new UAS
        _mav = uas;
        // Listen for system messages
        connect(UASMessageHandler::instance(), &UASMessageHandler::textMessageCountChanged, this, &MavManager::_handleTextMessage);
        // Now connect the new UAS
        connect(_mav, SIGNAL(attitudeChanged                    (UASInterface*,double,double,double,quint64)),              this, SLOT(_updateAttitude(UASInterface*, double, double, double, quint64)));
        connect(_mav, SIGNAL(attitudeChanged                    (UASInterface*,int,double,double,double,quint64)),          this, SLOT(_updateAttitude(UASInterface*,int,double, double, double, quint64)));
        connect(_mav, SIGNAL(speedChanged                       (UASInterface*, double, double, quint64)),                  this, SLOT(_updateSpeed(UASInterface*, double, double, quint64)));
        connect(_mav, SIGNAL(altitudeChanged                    (UASInterface*, double, double, double, double, quint64)),  this, SLOT(_updateAltitude(UASInterface*, double, double, double, double, quint64)));
        connect(_mav, SIGNAL(navigationControllerErrorsChanged  (UASInterface*, double, double, double)),                   this, SLOT(_updateNavigationControllerErrors(UASInterface*, double, double, double)));
        connect(_mav, SIGNAL(statusChanged                      (UASInterface*,QString,QString)),                           this, SLOT(_updateState(UASInterface*, QString,QString)));
        connect(_mav, SIGNAL(armingChanged                      (bool)),                                                    this, SLOT(_updateArmingState(bool)));
        connect(_mav, &UASInterface::NavigationControllerDataChanged,   this, &MavManager::_updateNavigationControllerData);
        connect(_mav, &UASInterface::heartbeatTimeout,                  this, &MavManager::_heartbeatTimeout);
        connect(_mav, &UASInterface::batteryChanged,                    this, &MavManager::_updateBatteryRemaining);
        connect(_mav, &UASInterface::batteryConsumedChanged,            this, &MavManager::_updateBatteryConsumedChanged);
        connect(_mav, &UASInterface::modeChanged,                       this, &MavManager::_updateMode);
        connect(_mav, &UASInterface::nameChanged,                       this, &MavManager::_updateName);
        connect(_mav, &UASInterface::systemTypeSet,                     this, &MavManager::_setSystemType);
        connect(_mav, &UASInterface::localizationChanged,               this, &MavManager::_setSatLoc);
        _wpm = _mav->getWaypointManager();
        if (_wpm) {
            connect(_wpm, &UASWaypointManager::currentWaypointChanged,   this, &MavManager::_updateCurrentWaypoint);
            connect(_wpm, &UASWaypointManager::waypointDistanceChanged,  this, &MavManager::_updateWaypointDistance);
            connect(_wpm, SIGNAL(waypointViewOnlyListChanged(void)),     this, SLOT(_waypointViewOnlyListChanged(void)));
            connect(_wpm, SIGNAL(waypointViewOnlyChanged(int,Waypoint*)),this, SLOT(_updateWaypointViewOnly(int,Waypoint*)));
            _wpm->readWaypoints(true);
        }
        UAS* pUas = dynamic_cast<UAS*>(_mav);
        if(pUas) {
            _setSatelliteCount(pUas->getSatelliteCount(), QString(""));
            connect(pUas, &UAS::satelliteCountChanged, this, &MavManager::_setSatelliteCount);
        }
        _setSystemType(_mav, _mav->getSystemType());
        _updateArmingState(_mav->isArmed());
    }
    emit mavPresentChanged();
}

void MavManager::_updateAttitude(UASInterface*, double roll, double pitch, double yaw, quint64)
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

void MavManager::_updateAttitude(UASInterface* uas, int, double roll, double pitch, double yaw, quint64 timestamp)
{
    _updateAttitude(uas, roll, pitch, yaw, timestamp);
}

void MavManager::_updateSpeed(UASInterface*, double groundSpeed, double airSpeed, quint64)
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

void MavManager::_updateAltitude(UASInterface*, double altitudeAMSL, double altitudeWGS84, double altitudeRelative, double climbRate, quint64) {
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

void MavManager::_updateNavigationControllerErrors(UASInterface*, double altitudeError, double speedError, double xtrackError) {
    _navigationAltitudeError   = altitudeError;
    _navigationSpeedError      = speedError;
    _navigationCrosstrackError = xtrackError;
}

void MavManager::_updateNavigationControllerData(UASInterface *uas, float, float, float, float targetBearing, float) {
    if (_mav == uas) {
        _navigationTargetBearing = targetBearing;
    }
}

/*
 * Internal
 */

bool MavManager::_isAirplane() {
    if (_mav)
        return _mav->isAirplane();
    return false;
}

void MavManager::_addChange(int id)
{
    if(!_changes.contains(id)) {
        _changes.append(id);
    }
}

float MavManager::_oneDecimal(float value)
{
    int i = (value * 10);
    return (float)i / 10.0;
}

void MavManager::_checkUpdate()
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
    // The timer rate is 20Hz for the coordinates above. These below we only check
    // twice a second.
    if(++_updateCount > 9) {
        _updateCount = 0;
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
}

QString MavManager::getMavIconColor()
{
    // TODO: Not using because not only the colors are ghastly, it doesn't respect dark/light palette
    if(_mav)
        return _mav->getColor().name();
    else
        return QString("black");
}

void MavManager::_updateArmingState(bool armed)
{
    if(_systemArmed != armed) {
        _systemArmed = armed;
        emit systemArmedChanged();
    }
}

void MavManager::_updateBatteryRemaining(UASInterface*, double voltage, double, double percent, int)
{

    if(percent < 0.0) {
        percent = 0.0;
    }
    if(voltage < 0.0) {
        voltage = 0.0;
    }
    if (_batteryVoltage != voltage) {
        _batteryVoltage = voltage;
        emit batteryVoltageChanged();
    }
    if (_batteryPercent != percent) {
        _batteryPercent = percent;
        emit batteryPercentChanged();
    }
}

void MavManager::_updateBatteryConsumedChanged(UASInterface*, double current_consumed)
{
    if(_batteryConsumed != current_consumed) {
        _batteryConsumed = current_consumed;
        emit batteryConsumedChanged();
    }
}


void MavManager::_updateState(UASInterface*, QString name, QString)
{
    if (_currentState != name) {
        _currentState = name;
        emit currentStateChanged();
    }
}

void MavManager::_updateMode(int, QString name, QString)
{
    if (name.size()) {
        QString shortMode = name;
        shortMode = shortMode.replace("D|", "");
        shortMode = shortMode.replace("A|", "");
        if (_currentMode != shortMode) {
            _currentMode = shortMode;
            emit currentModeChanged();
        }
    }
}

void MavManager::_updateName(const QString& name)
{
    if (_systemName != name) {
        _systemName = name;
        emit systemNameChanged();
    }
}

/**
 * The current system type is represented through the system icon.
 *
 * @param uas Source system, has to be the same as this->uas
 * @param systemType type ID, following the MAVLink system type conventions
 * @see http://pixhawk.ethz.ch/software/mavlink
 */
void MavManager::_setSystemType(UASInterface*, unsigned int systemType)
{
    _systemPixmap = "qrc:/res/mavs/";
    switch (systemType) {
        case MAV_TYPE_GENERIC:
            _systemPixmap += "Generic";
            break;
        case MAV_TYPE_FIXED_WING:
            _systemPixmap += "FixedWing";
            break;
        case MAV_TYPE_QUADROTOR:
            _systemPixmap += "QuadRotor";
            break;
        case MAV_TYPE_COAXIAL:
            _systemPixmap += "Coaxial";
            break;
        case MAV_TYPE_HELICOPTER:
            _systemPixmap += "Helicopter";
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            _systemPixmap += "AntennaTracker";
            break;
        case MAV_TYPE_GCS:
            _systemPixmap += "Groundstation";
            break;
        case MAV_TYPE_AIRSHIP:
            _systemPixmap += "Airship";
            break;
        case MAV_TYPE_FREE_BALLOON:
            _systemPixmap += "FreeBalloon";
            break;
        case MAV_TYPE_ROCKET:
            _systemPixmap += "Rocket";
            break;
        case MAV_TYPE_GROUND_ROVER:
            _systemPixmap += "GroundRover";
            break;
        case MAV_TYPE_SURFACE_BOAT:
            _systemPixmap += "SurfaceBoat";
            break;
        case MAV_TYPE_SUBMARINE:
            _systemPixmap += "Submarine";
            break;
        case MAV_TYPE_HEXAROTOR:
            _systemPixmap += "HexaRotor";
            break;
        case MAV_TYPE_OCTOROTOR:
            _systemPixmap += "OctoRotor";
            break;
        case MAV_TYPE_TRICOPTER:
            _systemPixmap += "TriCopter";
            break;
        case MAV_TYPE_FLAPPING_WING:
            _systemPixmap += "FlappingWing";
            break;
        case MAV_TYPE_KITE:
            _systemPixmap += "Kite";
            break;
        default:
            _systemPixmap += "Unknown";
            break;
    }
    emit systemPixmapChanged();
}

void MavManager::_heartbeatTimeout(bool timeout, unsigned int ms)
{
    unsigned int elapsed = ms;
    if (!timeout)
    {
        elapsed = 0;
    }
    if(elapsed != _currentHeartbeatTimeout) {
        _currentHeartbeatTimeout = elapsed;
        emit heartbeatTimeoutChanged();
    }
}

void MavManager::_setSatelliteCount(double val, QString)
{
    // I'm assuming that a negative value or over 99 means there is no GPS
    if(val < 0.0)  val = -1.0;
    if(val > 99.0) val = -1.0;
    if(_satelliteCount != (int)val) {
        _satelliteCount = (int)val;
        emit satelliteCountChanged();
    }
}

void MavManager::_setSatLoc(UASInterface*, int fix)
{
    // fix 0: lost, 1: at least one satellite, but no GPS fix, 2: 2D lock, 3: 3D lock
    if(_satelliteLock != fix) {
        _satelliteLock = fix;
        emit satelliteLockChanged();
    }
}

void MavManager::_updateWaypointDistance(double distance)
{
    if (_waypointDistance != distance) {
        _waypointDistance = distance;
        emit waypointDistanceChanged();
    }
}

void MavManager::_updateCurrentWaypoint(quint16 id)
{
    if (_currentWaypoint != id) {
        _currentWaypoint = id;
        emit currentWaypointChanged();
    }
}

void MavManager::_updateWaypointViewOnly(int, Waypoint* /*wp*/)
{
    /*
    bool changed = false;
    for(int i = 0; i < _waypoints.count(); i++) {
        if(_waypoints[i].getId() == wp->getId()) {
            _waypoints[i] = *wp;
            changed = true;
            break;
        }
    }
    if(changed) {
        emit waypointListChanged();
    }
    */
}

void MavManager::_waypointViewOnlyListChanged()
{
    if(_wpm) {
        const QList<Waypoint*> &waypoints = _wpm->getWaypointViewOnlyList();
        _waypoints.clear();
        for(int i = 0; i < waypoints.count(); i++) {
            Waypoint* wp = waypoints[i];
            _waypoints.append(new Waypoint(*wp));
        }
        emit waypointsChanged();
        /*
        if(_longitude == DEFAULT_LON && _latitude == DEFAULT_LAT && _waypoints.length()) {
            _longitude = _waypoints[0]->getLongitude();
            _latitude  = _waypoints[0]->getLatitude();
            emit longitudeChanged();
            emit latitudeChanged();
        }
        */
    }
}

void MavManager::_handleTextMessage(int newCount)
{
    // Reset?
    if(!newCount) {
        _currentMessageCount = 0;
        _currentNormalCount  = 0;
        _currentWarningCount = 0;
        _currentErrorCount   = 0;
        _messageCount        = 0;
        _currentMessageType  = MessageNone;
        emit newMessageCountChanged();
        emit messageTypeChanged();
        emit messageCountChanged();
        return;
    }

    UASMessageHandler* pMh = UASMessageHandler::instance();
    Q_ASSERT(pMh);
    MessageType_t type = newCount ? _currentMessageType : MessageNone;
    int errorCount     = _currentErrorCount;
    int warnCount      = _currentWarningCount;
    int normalCount    = _currentNormalCount;
    //-- Add current message counts
    errorCount  += pMh->getErrorCount();
    warnCount   += pMh->getWarningCount();
    normalCount += pMh->getNormalCount();
    //-- See if we have a higher level
    if(errorCount != _currentErrorCount) {
        _currentErrorCount = errorCount;
        type = MessageError;
    }
    if(warnCount != _currentWarningCount) {
        _currentWarningCount = warnCount;
        if(_currentMessageType != MessageError) {
            type = MessageWarning;
        }
    }
    if(normalCount != _currentNormalCount) {
        _currentNormalCount = normalCount;
        if(_currentMessageType != MessageError && _currentMessageType != MessageWarning) {
            type = MessageNormal;
        }
    }
    int count = _currentErrorCount + _currentWarningCount + _currentNormalCount;
    if(count != _currentMessageCount) {
        _currentMessageCount = count;
        // Display current total new messages count
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        _currentMessageType = type;
        // Update message level
        emit messageTypeChanged();
    }
    // Update message count (all messages)
    if(newCount != _messageCount) {
        _messageCount = newCount;
        emit messageCountChanged();
    }
    QString errMsg = pMh->getLatestError();
    if(errMsg != _latestError) {
        _latestError = errMsg;
        emit latestErrorChanged();
    }
}

void MavManager::resetMessages()
{
    // Reset Counts
    int count = _currentMessageCount;
    MessageType_t type = _currentMessageType;
    _currentErrorCount   = 0;
    _currentWarningCount = 0;
    _currentNormalCount  = 0;
    _currentMessageCount = 0;
    _currentMessageType = MessageNone;
    if(count != _currentMessageCount) {
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        emit messageTypeChanged();
    }
}
