/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RemoteIDManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "RemoteIDSettings.h"
#include "QGCQGeoCoordinate.h"
#include "PositionManager.h"

#include <QDebug>

QGC_LOGGING_CATEGORY(RemoteIDManagerLog, "RemoteIDManagerLog")

#define AREA_COUNT 1
#define AREA_RADIUS 0
#define SENDING_RATE_MSEC 1000
#define ALLOWED_GPS_DELAY 5000
#define RID_TIMEOUT 2500 // Messages should be arriving at 1 Hz, so we set a 2 second timeout

const uint8_t* RemoteIDManager::_id_or_mac_unknown = {NULL};

RemoteIDManager::RemoteIDManager(Vehicle* vehicle)
    : QObject               (vehicle)
    , _mavlink              (nullptr)
    , _vehicle              (vehicle)
    , _settings             (nullptr)
    , _armStatusGood        (false)
    , _commsGood            (false)
    , _gcsGPSGood           (false)
    , _basicIDGood          (true)
    , _GCSBasicIDValid      (false)
    , _operatorIDGood       (false)
    , _emergencyDeclared    (false)
    , _targetSystem         (0) // By default 0 means broadcast
    , _targetComponent      (0) // By default 0 means broadcast
    , _enforceSendingSelfID (false)
{
    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    _settings = qgcApp()->toolbox()->settingsManager()->remoteIDSettings();
    _positionManager = qgcApp()->toolbox()->qgcPositionManager();

    // Timer to track a healthy RID device. When expired we let the operator know
    _odidTimeoutTimer.setSingleShot(true);
    _odidTimeoutTimer.setInterval(RID_TIMEOUT);
    connect(&_odidTimeoutTimer, &QTimer::timeout, this, &RemoteIDManager::_odidTimeout);

    // Timer to send messages at a constant rate
    _sendMessagesTimer.setInterval(SENDING_RATE_MSEC);
    connect(&_sendMessagesTimer, &QTimer::timeout, this, &RemoteIDManager::_sendMessages);

    // GCS GPS position updates to track the health of the GPS data
    connect(_positionManager, &QGCPositionManager::positionInfoUpdated, this, &RemoteIDManager::_updateLastGCSPositionInfo);

    // Check changes in basic id settings as long as they are modified
    connect(_settings->basicID(), &Fact::rawValueChanged, this, &RemoteIDManager::_checkGCSBasicID);
    connect(_settings->basicIDType(), &Fact::rawValueChanged, this, &RemoteIDManager::_checkGCSBasicID);
    connect(_settings->basicIDUaType(), &Fact::rawValueChanged, this, &RemoteIDManager::_checkGCSBasicID);

    // Assign vehicle sysid and compid. GCS must target these messages to autopilot, and autopilot will redirect them to RID device
    _targetSystem = _vehicle->id();
    _targetComponent = _vehicle->compId();
}

void RemoteIDManager::mavlinkMessageReceived(mavlink_message_t& message )
{
    switch (message.msgid) {
    // So far we are only listening to this one, as heartbeat won't be sent if connected by CAN
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_ARM_STATUS: 
        _handleArmStatus(message);
    default:
        break;
    }
}

// This slot will be called if we stop receiving heartbeats for more than RID_TIMEOUT seconds
void RemoteIDManager::_odidTimeout()
{
    _commsGood = false;
    _sendMessagesTimer.stop(); // We stop sending messages if the communication with the RID device is down
    emit commsGoodChanged();
    qCDebug(RemoteIDManagerLog) << "We stopped receiving heartbeat from RID device.";
}

// Parsing of the ARM_STATUS message comming from the RID device
void RemoteIDManager::_handleArmStatus(mavlink_message_t& message)
{
    // Compid must be ODID_TXRX_X
    if ( (message.compid < MAV_COMP_ID_ODID_TXRX_1) || (message.compid > MAV_COMP_ID_ODID_TXRX_3) ) {
        // or same as autopilot, in the case of Ardupilot and CAN RID modules
        if (message.compid != MAV_COMP_ID_AUTOPILOT1) {
            return;
        }
    }

    // Sanity check, only get messages from same sysid 
    if (_vehicle->id() != message.sysid) {
        return;
    }

    // We set the targetsystem
    if (_targetSystem != message.sysid) {
        _targetSystem = message.sysid;
        qCDebug(RemoteIDManagerLog) << "Subscribing to ODID messages coming from system " << _targetSystem;
    }

    if (!_commsGood) {
        _commsGood = true;
        _sendMessagesTimer.start();     // Start sending our messages
        _checkGCSBasicID();             // Check if basicID is good to send 
        checkOperatorID();              // Check if OperatorID is good in case we want to send it from start because of the settings
        emit commsGoodChanged();
        qCDebug(RemoteIDManagerLog) << "Receiving ODID_ARM_STATUS from RID device";
    }

    // Restart the timeout
    _odidTimeoutTimer.start();

    // CompId and sysId are correct, we can proceed
    mavlink_open_drone_id_arm_status_t armStatus;
    mavlink_msg_open_drone_id_arm_status_decode(&message, &armStatus);

    if (armStatus.status == MAV_ODID_GOOD_TO_ARM && !_armStatusGood) {
        // If good to arm, even if basic ID is not set on GCS, it was set by remoteID parameters, so GCS one would be optional in this case
        if (!_basicIDGood) {
            _basicIDGood = true;
            emit basicIDGoodChanged();
        }
        _armStatusGood = true;
        emit armStatusGoodChanged();
        qCDebug(RemoteIDManagerLog) << "Arm status GOOD TO ARM.";
    }

    if (armStatus.status == MAV_ODID_PRE_ARM_FAIL_GENERIC) {
        _armStatusGood = false;
        _armStatusError = QString::fromLocal8Bit(armStatus.error);
        // Check if the error is because of missing basic id
        if (armStatus.error == QString("missing basic_id message")) {
            _basicIDGood = false;
            qCDebug(RemoteIDManagerLog) << "Arm status error, basic_id is not set in RID device nor in GCS!";
            emit basicIDGoodChanged();
        }
        emit armStatusGoodChanged();
        emit armStatusErrorChanged();
        qCDebug(RemoteIDManagerLog) << "Arm status error:" << _armStatusError;
    }
}

// Function that sends messages periodically
void RemoteIDManager::_sendMessages()
{
    // We only send RemoteID messages if we have it enabled in General settings
    if (!_settings->enable()->rawValue().toBool()) {
        return;
    }
    
    // We always try to send System
    _sendSystem();
    
    // only send it if the information is correct and the tickbox in settings is set
    if (_GCSBasicIDValid && _settings->sendBasicID()->rawValue().toBool()) {
        _sendBasicID();
    }
    
    // We only send selfID if the pilot wants it or in case of a declared emergency. If an emergency is cleared
    // we also keep sending the message, to be sure the non emergency state makes it up to the vehicle
    if (_settings->sendSelfID()->rawValue().toBool() || _emergencyDeclared || _enforceSendingSelfID) {
        _sendSelfIDMsg();
    }

    // We only send the OperatorID if the pilot wants it or if the region we have set is europe. 
    // To be able to send it, it needs to be filled correclty
    if ((_settings->sendOperatorID()->rawValue().toBool() || (_settings->region()->rawValue().toInt() == Region::EU)) && _operatorIDGood) {
        _sendOperatorID();
    }

}

void RemoteIDManager::_sendSelfIDMsg()
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    SharedLinkInterfacePtr sharedLink = weakLink.lock();

    if (sharedLink) {
        mavlink_message_t msg;

        mavlink_msg_open_drone_id_self_id_pack_chan(_mavlink->getSystemId(),
                                                    _mavlink->getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    _targetSystem,
                                                    _targetComponent,
                                                    _id_or_mac_unknown,
                                                    _emergencyDeclared ? 1 : _settings->selfIDType()->rawValue().toInt(), // If emergency is delcared we send directly a 1 (1 = EMERGENCY)
                                                    _getSelfIDDescription()); // Depending on the type of SelfID we send a different description
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

// We need to return the correct description for the self ID type we have selected
const char* RemoteIDManager::_getSelfIDDescription()
{
    QByteArray bytesFree = (_settings->selfIDFree()->rawValue().toString()).toLocal8Bit();
    QByteArray bytesEmergency = (_settings->selfIDEmergency()->rawValue().toString()).toLocal8Bit();
    QByteArray bytesExtended = (_settings->selfIDExtended()->rawValue().toString()).toLocal8Bit();

    const char* descriptionToSend;

    if (_emergencyDeclared) {
        // If emergency is declared we dont care about the settings and we send emergency directly
        descriptionToSend = bytesEmergency.data();
    } else {
        switch (_settings->selfIDType()->rawValue().toInt()) {
            case 0:
                descriptionToSend = bytesFree.data();
                break;
            case 1:
                descriptionToSend = bytesEmergency.data();
                break;
            case 2:
                descriptionToSend = bytesExtended.data();
                break;
            default:
                descriptionToSend = bytesEmergency.data();
        }
    }
    
    return descriptionToSend;
}

void RemoteIDManager::_sendOperatorID()
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    SharedLinkInterfacePtr sharedLink = weakLink.lock();

    if (sharedLink) {
        mavlink_message_t msg;

        QByteArray bytesOperatorID = (_settings->operatorID()->rawValue().toString()).toLocal8Bit();
        const char* descriptionToSend = bytesOperatorID.data();

        mavlink_msg_open_drone_id_operator_id_pack_chan(
                                                    _mavlink->getSystemId(),
                                                    _mavlink->getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    _targetSystem,
                                                    _targetComponent,
                                                    _id_or_mac_unknown,
                                                    _settings->operatorIDType()->rawValue().toInt(),
                                                    descriptionToSend);

        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void RemoteIDManager::_sendSystem()
{
    QGeoCoordinate      gcsPosition;
    QGeoPositionInfo    geoPositionInfo;
    // Location types:
    // 0 -> TAKEOFF (not supported yet)
    // 1 -> LIVE GNNS 
    // 2 -> FIXED
    if (_settings->locationType()->rawValue().toUInt() == LocationTypes::FIXED) {
        // For FIXED location, we first check that the values are valid. Then we populate our position
        if (_settings->latitudeFixed()->rawValue().toFloat() >= -90 && _settings->latitudeFixed()->rawValue().toFloat() <= 90 && _settings->longitudeFixed()->rawValue().toFloat() >= -180 && _settings->longitudeFixed()->rawValue().toFloat() <= 180) {
            gcsPosition = QGeoCoordinate(_settings->latitudeFixed()->rawValue().toFloat(), _settings->longitudeFixed()->rawValue().toFloat(), _settings->altitudeFixed()->rawValue().toFloat());
            geoPositionInfo = QGeoPositionInfo(gcsPosition, QDateTime::currentDateTime().currentDateTimeUtc());
            if (!_gcsGPSGood) {
                _gcsGPSGood = true;
                emit gcsGPSGoodChanged();
            }
        } else {
            gcsPosition = QGeoCoordinate(0,0,0);
            geoPositionInfo = QGeoPositionInfo(gcsPosition, QDateTime::currentDateTime().currentDateTimeUtc());
            if (_gcsGPSGood) {
                _gcsGPSGood = false;
                emit gcsGPSGoodChanged();
                qCDebug(RemoteIDManagerLog) << "The provided coordinates for FIXED position are invalid.";
            }
        }
    } else {
        // For Live GNSS we take QGC GPS data
        gcsPosition = _positionManager->gcsPosition();
        geoPositionInfo = _positionManager->geoPositionInfo();

        // GPS position needs to be valid before checking other stuff
        if (geoPositionInfo.isValid()) {
            // If we dont have altitude for FAA then the GPS data is no good
            if ((_settings->region()->rawValue().toInt() == Region::FAA) && !(gcsPosition.altitude() >= 0) && _gcsGPSGood) {
                _gcsGPSGood = false;
                emit gcsGPSGoodChanged();
                qCDebug(RemoteIDManagerLog) << "GCS GPS data error (no altitude): Altitude data is mandatory for GCS GPS data in FAA regions.";
                return;
            }

            // If the GPS data is older than ALLOWED_GPS_DELAY we cannot use this data 
            if (_lastGeoPositionTimeStamp.msecsTo(QDateTime::currentDateTime().currentDateTimeUtc()) > ALLOWED_GPS_DELAY) {
                if (_gcsGPSGood) {
                    _gcsGPSGood = false;
                    emit gcsGPSGoodChanged();
                    qCDebug(RemoteIDManagerLog) << "GCS GPS data is older than 5 seconds";
                }
            } else {
                if (!_gcsGPSGood) {
                    _gcsGPSGood = true;
                    emit gcsGPSGoodChanged();
                }
            }
        } else {
            _gcsGPSGood = false;
            emit gcsGPSGoodChanged();
            qCDebug(RemoteIDManagerLog) << "GCS GPS data is not valid.";
        }
        
    }

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    SharedLinkInterfacePtr sharedLink = weakLink.lock();
    
    if (sharedLink) {
        mavlink_message_t msg;

        mavlink_msg_open_drone_id_system_pack_chan(_mavlink->getSystemId(),
                                                    _mavlink->getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    _targetSystem,
                                                    _targetComponent,
                                                    _id_or_mac_unknown,
                                                    _settings->locationType()->rawValue().toUInt(),
                                                    _settings->classificationType()->rawValue().toUInt(),
                                                    _gcsGPSGood ? ( gcsPosition.latitude()  * 1.0e7 ) : 0, // If position not valid, send a 0
                                                    _gcsGPSGood ? ( gcsPosition.longitude() * 1.0e7 ) : 0, // If position not valid, send a 0
                                                    AREA_COUNT,
                                                    AREA_RADIUS,
                                                    -1000.0f,
                                                    -1000.0f,
                                                    _settings->categoryEU()->rawValue().toUInt(),
                                                    _settings->classEU()->rawValue().toUInt(),
                                                    _gcsGPSGood ? gcsPosition.altitude() : 0, // If position not valid, send a 0
                                                    _timestamp2019()), // Time stamp needs to be since 00:00:00 1/1/2019
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

// Returns seconds elapsed since 00:00:00 1/1/2019
uint32_t RemoteIDManager::_timestamp2019()
{
    uint32_t secsSinceEpoch2019 = 1546300800; // Secs elapsed since epoch to 1-1-2019

    return ((QDateTime::currentDateTime().currentSecsSinceEpoch()) - secsSinceEpoch2019);
}

void RemoteIDManager::_sendBasicID()
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    SharedLinkInterfacePtr sharedLink = weakLink.lock();

    if (sharedLink) {
        mavlink_message_t msg;
        
        QString basicIDTemp = _settings->basicID()->rawValue().toString();
        QByteArray ba = basicIDTemp.toLocal8Bit();
        // To make sure the buffer is large enough to fit the message. It will add padding bytes if smaller, or exclude the extra ones if bigger 
        ba.resize(MAVLINK_MSG_OPEN_DRONE_ID_BASIC_ID_FIELD_UAS_ID_LEN);

        mavlink_msg_open_drone_id_basic_id_pack_chan(_mavlink->getSystemId(),
                                                    _mavlink->getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    _targetSystem,
                                                    _targetComponent,
                                                    _id_or_mac_unknown,
                                                    _settings->basicIDType()->rawValue().toUInt(),
                                                    _settings->basicIDUaType()->rawValue().toUInt(),
                                                    reinterpret_cast<const unsigned char*>(ba.constData())),

        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

void RemoteIDManager::_checkGCSBasicID()
{
    QString basicID = _settings->basicID()->rawValue().toString();

    if (!basicID.isEmpty() && (_settings->basicIDType()->rawValue().toInt() >= 0) && (_settings->basicIDUaType()->rawValue().toInt() >= 0)) {
        _GCSBasicIDValid = true;
    } else {
        _GCSBasicIDValid = false;
    }
}

void RemoteIDManager::checkOperatorID()
{
    QString operatorID = _settings->operatorID()->rawValue().toString();

    if (!operatorID.isEmpty() && (_settings->operatorIDType()->rawValue().toInt() >= 0)) {
        _operatorIDGood = true;
    } else {
        _operatorIDGood = false;
    }
    emit operatorIDGoodChanged();
}

void RemoteIDManager::setEmergency(bool declare)
{
    _emergencyDeclared = declare;
    emit emergencyDeclaredChanged();
    // Wether we are starting an emergency or cancelling it, we need to enforce sending
    // this message. Otherwise, if non optimal connection quality, vehicle RID device
    // could remain in the wrong state. It is clarified to the user in remoteidsettings.qml
    _enforceSendingSelfID = true;

    qCDebug(RemoteIDManagerLog) << ( declare ? "Emergency declared." : "Emergency cleared.");
}

void RemoteIDManager::_updateLastGCSPositionInfo(QGeoPositionInfo update)
{
    if (update.isValid()) {
        _lastGeoPositionTimeStamp = update.timestamp().toUTC();
    }
}