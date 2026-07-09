#include "RemoteIDManager.h"
#include "MAVLinkLib.h"
#include "SettingsManager.h"
#include "RemoteIDSettings.h"
#include "PositionManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RemoteIDManagerLog, "Vehicle.RemoteIDManager")

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
    , _armStatusGoodToArm   (false)
    , _ridDeviceCommsGood   (false)
    , _gcsPositionUsable    (false)
    , _vehicleReportsBasicIDMissing(false)
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
    _ridDeviceCommsGood = false;
    _sendMessagesTimer.stop(); // We stop sending messages if the communication with the RID device is down
    emit ridDeviceCommsGoodChanged();
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

    if (!_ridDeviceCommsGood) {
        _ridDeviceCommsGood = true;
        _sendMessagesTimer.start();     // Start sending our messages
        emit ridDeviceCommsGoodChanged();
        qCDebug(RemoteIDManagerLog) << "Receiving ODID_ARM_STATUS from RID device";
    }

    // Restart the timeout
    _odidTimeoutTimer.start();

    // CompId and sysId are correct, we can proceed
    mavlink_open_drone_id_arm_status_t armStatus;
    mavlink_msg_open_drone_id_arm_status_decode(&message, &armStatus);

    if (armStatus.status == MAV_ODID_ARM_STATUS_GOOD_TO_ARM) {
        // If good to arm, even if basic ID is not set on GCS, it was set by remoteID parameters, so GCS one would be optional in this case
        if (_vehicleReportsBasicIDMissing) {
            _vehicleReportsBasicIDMissing = false;
            emit vehicleReportsBasicIDMissingChanged();
        }
        // Clear any stale error text so the UI stops showing it once the device recovers
        if (!_armStatusError.isEmpty()) {
            _armStatusError.clear();
            emit armStatusErrorChanged();
        }
        if (!_armStatusGoodToArm) {
            _armStatusGoodToArm = true;
            emit armStatusGoodToArmChanged();
            qCDebug(RemoteIDManagerLog) << "Arm status GOOD TO ARM.";
        }
    }

    if (armStatus.status == MAV_ODID_ARM_STATUS_PRE_ARM_FAIL_GENERIC) {
        if (_armStatusGoodToArm) {
            _armStatusGoodToArm = false;
            emit armStatusGoodToArmChanged();
        }
        // MAVLink char fields are only null-terminated when shorter than the field, so bound the decode
        const QString armStatusError = QString::fromUtf8(armStatus.error, qstrnlen(armStatus.error, sizeof(armStatus.error)));
        // The flag tracks the current reported error: set while the device blames basic ID,
        // cleared when it fails for a different reason (don't leave the UI blaming basic ID)
        const bool basicIDMissing = (armStatusError == QStringLiteral("missing basic_id message"));
        if (_vehicleReportsBasicIDMissing != basicIDMissing) {
            _vehicleReportsBasicIDMissing = basicIDMissing;
            if (basicIDMissing) {
                qCDebug(RemoteIDManagerLog) << "Arm status error, basic_id is not set in RID device nor in GCS!";
            }
            emit vehicleReportsBasicIDMissingChanged();
        }
        if (_armStatusError != armStatusError) {
            _armStatusError = armStatusError;
            emit armStatusErrorChanged();
            qCDebug(RemoteIDManagerLog) << "Arm status error:" << _armStatusError;
        }
    }
}

// Function that sends messages periodically
void RemoteIDManager::_sendMessages()
{
    // We always try to send System
    _sendSystem();

    // only send it if the information is correct and the tickbox in settings is set
    if (_settings->basicIDValid() && _settings->sendBasicID()->rawValue().toBool()) {
        _sendBasicID();
    }

    // We only send selfID if the pilot wants it or in case of a declared emergency. If an emergency is cleared
    // we also keep sending the message, to be sure the non emergency state makes it up to the vehicle
    if (_settings->sendSelfID()->rawValue().toBool() || _emergencyDeclared || _enforceSendingSelfID) {
        _sendSelfIDMsg();
    }

    // We only send the OperatorID if the pilot wants it or if the region we have set is europe.
    // To be able to send it, it needs to be filled correclty
    if ((_settings->sendOperatorID()->rawValue().toBool() || (_settings->region()->rawValue().toInt() == static_cast<int>(RemoteIDSettings::RegionOperation::EU))) && _settings->operatorIDValidForRegion()) {
        _sendOperatorID();
    }

}

void RemoteIDManager::_sendSelfIDMsg()
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    SharedLinkInterfacePtr sharedLink = weakLink.lock();

    if (sharedLink) {
        mavlink_message_t msg;
        const QByteArray selfIdDescription = _getSelfIDDescription();

        mavlink_msg_open_drone_id_self_id_pack_chan(MAVLinkProtocol::instance()->getSystemId(),
                                                    MAVLinkProtocol::getComponentId(),
                                                    sharedLink->mavlinkChannel(),
                                                    &msg,
                                                    _targetSystem,
                                                    _targetComponent,
                                                    _id_or_mac_unknown,
                                                    _emergencyDeclared ? 1 : _settings->selfIDType()->rawValue().toInt(), // If emergency is delcared we send directly a 1 (1 = EMERGENCY)
                                                    selfIdDescription.constData()); // Depending on the type of SelfID we send a different description
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

// We need to return the correct description for the self ID type we have selected
QByteArray RemoteIDManager::_getSelfIDDescription() const
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
    descriptionBuffer.resize(MAVLINK_MSG_OPEN_DRONE_ID_SELF_ID_FIELD_DESCRIPTION_LEN, '\0');
    return descriptionBuffer;
}

void RemoteIDManager::_sendOperatorID()
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    SharedLinkInterfacePtr sharedLink = weakLink.lock();

    if (sharedLink) {
        mavlink_message_t msg;

        // Each region stores its own operator ID; broadcast the one matching the selected region
        const bool isEURegion = (_settings->region()->rawValue().toInt() == static_cast<int>(RemoteIDSettings::RegionOperation::EU));
        Fact* const operatorIDFact = isEURegion ? _settings->operatorIDEU() : _settings->operatorIDFAA();
        QByteArray bytesOperatorID = operatorIDFact->rawValue().toString().toLocal8Bit();
        bytesOperatorID.resize(MAVLINK_MSG_OPEN_DRONE_ID_OPERATOR_ID_FIELD_OPERATOR_ID_LEN, '\0');

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

void RemoteIDManager::_updateGcsPositionStatus(bool usable, const QString& error)
{
    if (!error.isEmpty() && _gcsPositionError != error) {
        _gcsPositionError = error;
        qCWarning(RemoteIDManagerLog) << "GCS GPS error:" << error;
    }
    if (_gcsPositionUsable != usable) {
        _gcsPositionUsable = usable;
        if (usable) {
            _gcsPositionError.clear();
        }
        emit gcsPositionUsableChanged();
    }
}

void RemoteIDManager::_sendSystem()
{
    QGeoCoordinate gcsPosition(0, 0, 0);
    const uint32_t locationType = _settings->locationType()->rawValue().toUInt();
    // Location types:
    // 0 -> TAKEOFF (not supported yet)
    // 1 -> LIVE GNNS
    // 2 -> FIXED
    if (locationType == LocationTypes::FIXED) {
        const double lat = _settings->latitudeFixed()->rawValue().toDouble();
        const double lon = _settings->longitudeFixed()->rawValue().toDouble();
        const double alt = _settings->altitudeFixed()->rawValue().toDouble();

        // For FIXED location, we first check that the values are valid. Then we populate our position
        if (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0) {
            gcsPosition = QGeoCoordinate(lat, lon, alt);
            _updateGcsPositionStatus(true);
        } else {
            _updateGcsPositionStatus(false, "The provided coordinates for FIXED position are invalid.");
        }
    } else {
        QGCPositionManager* positionManager = QGCPositionManager::instance();
        QGeoPositionInfo geoPositionInfo = positionManager->geoPositionInfo();
        gcsPosition = positionManager->gcsPosition();

        if (!geoPositionInfo.isValid()) {
            // Only warn if we've previously received a valid fix; otherwise the source is
            // still initializing and the absence of data is expected, not an error.
            _updateGcsPositionStatus(false, _lastGeoPositionTimeStamp.isValid()
                                            ? QStringLiteral("GCS GPS data is not valid.")
                                            : QString());
        } else if (positionManager->gcsPositioningError() != QGeoPositionInfoSource::NoError && positionManager->gcsPositioningError() != QGeoPositionInfoSource::UpdateTimeoutError) {
            _updateGcsPositionStatus(false, QString("GCS GPS data error: %1").arg(positionManager->gcsPositioningError()));
        } else if (!gcsPosition.isValid() || gcsPosition.type() == QGeoCoordinate::InvalidCoordinate) {
            _updateGcsPositionStatus(false, "GCS GPS data error: Invalid coordinate type.");
        } else if (_settings->region()->rawValue().toInt() == static_cast<int>(RemoteIDSettings::RegionOperation::FAA) && gcsPosition.type() != QGeoCoordinate::Coordinate3D) {
            // FAA requires altitude data, or else the GPS data is not good
            _updateGcsPositionStatus(false, "GCS GPS data error: Altitude data is mandatory for FAA regions.");
        } else if (_lastGeoPositionTimeStamp.msecsTo(QDateTime::currentDateTime().currentDateTimeUtc()) > ALLOWED_GPS_DELAY) {
            _updateGcsPositionStatus(false, "GCS GPS data is older than 5 seconds");
        } else {
            _updateGcsPositionStatus(true);
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
                                                    _gcsPositionUsable ? ( gcsPosition.latitude()  * 1.0e7 ) : MAVLINK_UNKNOWN_LAT,
                                                    _gcsPositionUsable ? ( gcsPosition.longitude() * 1.0e7 ) : MAVLINK_UNKNOWN_LON,
                                                    AREA_COUNT,
                                                    AREA_RADIUS,
                                                    MAVLINK_UNKNOWN_METERS,
                                                    MAVLINK_UNKNOWN_METERS,
                                                    _settings->categoryEU()->rawValue().toUInt(),
                                                    _settings->classEU()->rawValue().toUInt(),
                                                    _gcsPositionUsable ? gcsPosition.altitude() : MAVLINK_UNKNOWN_METERS,
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
        ba.resize(MAVLINK_MSG_OPEN_DRONE_ID_BASIC_ID_FIELD_UAS_ID_LEN, '\0');

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
