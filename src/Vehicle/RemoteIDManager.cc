/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RemoteIDManager.h"
#include "SettingsManager.h"
#include "RemoteIDSettings.h"
#include "PositionManager.h"
#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RemoteIDManagerLog, "RemoteIDManagerLog")

#define AREA_COUNT 1
#define AREA_RADIUS 0
#define MAVLINK_UNKNOWN_METERS -1000.0f
#define MAVLINK_UNKNOWN_LAT 0
#define MAVLINK_UNKNOWN_LON 0
#define SENDING_RATE_MSEC 1000
#define ALLOWED_GPS_DELAY 5000
#define RID_TIMEOUT 2500 // Messages should be arriving at 1 Hz, so we set a 2 second timeout

const uint8_t* RemoteIDManager::_id_or_mac_unknown = new uint8_t[MAVLINK_MSG_OPEN_DRONE_ID_OPERATOR_ID_FIELD_ID_OR_MAC_LEN]();

RemoteIDManager::RemoteIDManager(Vehicle* vehicle)
    : QObject               (vehicle)
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
    _settings = SettingsManager::instance()->remoteIDSettings();

    // Timer to track a healthy RID device. When expired we let the operator know
    _odidTimeoutTimer.setSingleShot(true);
    _odidTimeoutTimer.setInterval(RID_TIMEOUT);
    connect(&_odidTimeoutTimer, &QTimer::timeout, this, &RemoteIDManager::_odidTimeout);

    // Timer to send messages at a constant rate
    _sendMessagesTimer.setInterval(SENDING_RATE_MSEC);
    connect(&_sendMessagesTimer, &QTimer::timeout, this, &RemoteIDManager::_sendMessages);

    // GCS GPS position updates to track the health of the GPS data
    connect(QGCPositionManager::instance(), &QGCPositionManager::positionInfoUpdated, this, &RemoteIDManager::_updateLastGCSPositionInfo);

    // Check changes in basic id settings as long as they are modified
    connect(_settings->basicID(), &Fact::rawValueChanged, this, &RemoteIDManager::_checkGCSBasicID);
    connect(_settings->basicIDType(), &Fact::rawValueChanged, this, &RemoteIDManager::_checkGCSBasicID);
    connect(_settings->basicIDUaType(), &Fact::rawValueChanged, this, &RemoteIDManager::_checkGCSBasicID);

    // Assign vehicle sysid and compid. GCS must target these messages to autopilot, and autopilot will redirect them to RID device
    _targetSystem = _vehicle->id();
    _targetComponent = _vehicle->compId();

    if (_settings->operatorIDValid()->rawValue() == true) {
        // If it was already checked, we can flag this as good to go.
        // We don't do a fresh verification because we don't store the private part of the ID.
        _operatorIDGood = true;
        operatorIDGoodChanged();
    }
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

    if (!_available) {
        _available = true;
        emit availableChanged();
        qCDebug(RemoteIDManagerLog) << "Receiving ODID_ARM_STATUS for first time. Mavlink Open Drone ID support is available.";
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
        emit commsGoodChanged();
        qCDebug(RemoteIDManagerLog) << "Receiving ODID_ARM_STATUS from RID device";
    }

    // Restart the timeout
    _odidTimeoutTimer.start();

    // CompId and sysId are correct, we can proceed
    mavlink_open_drone_id_arm_status_t armStatus;
    mavlink_msg_open_drone_id_arm_status_decode(&message, &armStatus);

    if (armStatus.status == MAV_ODID_ARM_STATUS_GOOD_TO_ARM && !_armStatusGood) {
        // If good to arm, even if basic ID is not set on GCS, it was set by remoteID parameters, so GCS one would be optional in this case
        if (!_basicIDGood) {
            _basicIDGood = true;
            emit basicIDGoodChanged();
        }
        _armStatusGood = true;
        emit armStatusGoodChanged();
        qCDebug(RemoteIDManagerLog) << "Arm status GOOD TO ARM.";
    }

    if (armStatus.status == MAV_ODID_ARM_STATUS_PRE_ARM_FAIL_GENERIC) {
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

        mavlink_msg_open_drone_id_self_id_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                    MAVLinkProtocol::getComponentId(),
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
    QString descriptionToSend;

    if (_emergencyDeclared) {
        // If emergency is declared we dont care about the settings and we send emergency directly
        descriptionToSend = _settings->selfIDEmergency()->rawValue().toString();
    } else {
        switch (_settings->selfIDType()->rawValue().toInt()) {
            case 0:
                descriptionToSend = _settings->selfIDFree()->rawValue().toString();
                break;
            case 1:
                descriptionToSend = _settings->selfIDEmergency()->rawValue().toString();
                break;
            case 2:
                descriptionToSend = _settings->selfIDExtended()->rawValue().toString();
                break;
            default:
                descriptionToSend = _settings->selfIDEmergency()->rawValue().toString();
        }
    }

    QByteArray descriptionBuffer = descriptionToSend.toLocal8Bit();
    descriptionBuffer.resize(MAVLINK_MSG_OPEN_DRONE_ID_SELF_ID_FIELD_DESCRIPTION_LEN);
    return descriptionBuffer.constData();
}

void RemoteIDManager::_sendOperatorID()
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    SharedLinkInterfacePtr sharedLink = weakLink.lock();

    if (sharedLink) {
        mavlink_message_t msg;

        QByteArray bytesOperatorID = (_settings->operatorID()->rawValue().toString()).toLocal8Bit();
        bytesOperatorID.resize(MAVLINK_MSG_OPEN_DRONE_ID_OPERATOR_ID_FIELD_OPERATOR_ID_LEN);

        mavlink_msg_open_drone_id_operator_id_pack_chan(
                                                    MAVLinkProtocol::instance()->getSystemId(),
                                                    MAVLinkProtocol::getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    _targetSystem,
                                                    _targetComponent,
                                                    _id_or_mac_unknown,
                                                    _settings->operatorIDType()->rawValue().toInt(),
                                                    bytesOperatorID.constData());

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
        gcsPosition = QGCPositionManager::instance()->gcsPosition();
        geoPositionInfo = QGCPositionManager::instance()->geoPositionInfo();

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

        mavlink_msg_open_drone_id_system_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                    MAVLinkProtocol::getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    _targetSystem,
                                                    _targetComponent,
                                                    _id_or_mac_unknown,
                                                    _settings->locationType()->rawValue().toUInt(),
                                                    _settings->classificationType()->rawValue().toUInt(),
                                                    _gcsGPSGood ? ( gcsPosition.latitude()  * 1.0e7 ) : MAVLINK_UNKNOWN_LAT,
                                                    _gcsGPSGood ? ( gcsPosition.longitude() * 1.0e7 ) : MAVLINK_UNKNOWN_LON,
                                                    AREA_COUNT,
                                                    AREA_RADIUS,
                                                    MAVLINK_UNKNOWN_METERS,
                                                    MAVLINK_UNKNOWN_METERS,
                                                    _settings->categoryEU()->rawValue().toUInt(),
                                                    _settings->classEU()->rawValue().toUInt(),
                                                    _gcsGPSGood ? gcsPosition.altitude() : MAVLINK_UNKNOWN_METERS,
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

        mavlink_msg_open_drone_id_basic_id_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                    MAVLinkProtocol::getComponentId(),
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

void RemoteIDManager::checkOperatorID(const QString& operatorID)
{
    // We overwrite the fact that is also set by the text input but we want to update
    // after every letter rather than when editing is done.
    // We check whether it actually changed to avoid triggering this on startup.
    if (operatorID != _settings->operatorID()->rawValueString()) {
        _settings->operatorIDValid()->setRawValue(_isEUOperatorIDValid(operatorID));
    }
}

void RemoteIDManager::setOperatorID()
{
    QString operatorID = _settings->operatorID()->rawValue().toString();

    if (_settings->region()->rawValue().toInt() == Region::EU) {
        // Save for next time because we don't save the private part,
        // so we can't re-verify next time and just trust the value
        // in the settings.
        _operatorIDGood = _settings->operatorIDValid()->rawValue() == true;
        if (_operatorIDGood) {
            // Strip private part
            _settings->operatorID()->setRawValue(operatorID.sliced(0, 16));
        }

    } else {
        // Otherwise, we just check if there is anything entered
        _operatorIDGood =
            (!operatorID.isEmpty() && (_settings->operatorIDType()->rawValue().toInt() >= 0));
    }

    emit operatorIDGoodChanged();
}

bool RemoteIDManager::_isEUOperatorIDValid(const QString& operatorID) const
{
    const bool containsDash = operatorID.contains('-');
    if (!(operatorID.length() == 20 && containsDash) && !(operatorID.length() == 19 && !containsDash)) {
        qCDebug(RemoteIDManagerLog) << "OperatorID not long enough";
        return false;
    }

    const QString countryCode = operatorID.sliced(0,3);
    if (!countryCode.isUpper()) {
        qCDebug(RemoteIDManagerLog) << "OperatorID country code not uppercase";
        return false;
    }

    const QString number = operatorID.sliced(3, 12);
    const QChar checksum = operatorID.at(15);
    const QString secret = containsDash ? operatorID.sliced(17, 3) : operatorID.sliced(16, 3);
    const QString combination = number + secret;

    const QChar result = _calculateLuhnMod36(combination);

    const bool valid = (result == checksum);
    qCDebug(RemoteIDManagerLog) << "Operator ID checksum " << (valid ? "valid" : "invalid");
    return valid;
}

QChar RemoteIDManager::_calculateLuhnMod36(const QString& input) const {
    const int n = 36;
    const QString alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";

    int sum = 0;
    int factor = 2;

    for (int i = input.length() - 1; i >= 0; i--) {
        int codePoint = alphabet.indexOf(input[i]);
        int addend = factor * codePoint;
        factor = (factor == 2) ? 1 : 2;
        addend = (addend / n) + (addend % n);
        sum += addend;
    }

    int remainder = sum % n;
    int checkCodePoint = (n - remainder) % n;
    return alphabet.at(checkCodePoint);
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
