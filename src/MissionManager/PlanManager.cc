#include "PlanManager.h"
#include "PlanManagerStateMachine.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(PlanManagerLog, "PlanManager.PlanManager")

PlanManager::PlanManager(Vehicle* vehicle, MAV_MISSION_TYPE planType)
    : QObject   (vehicle)
    , _vehicle  (vehicle)
    , _planType (planType)
{
    _stateMachine = new PlanManagerStateMachine(this, this);

    connect(_stateMachine, &PlanManagerStateMachine::readComplete, this, &PlanManager::_onReadComplete);
    connect(_stateMachine, &PlanManagerStateMachine::writeComplete, this, &PlanManager::_onWriteComplete);
    connect(_stateMachine, &PlanManagerStateMachine::removeAllComplete, this, &PlanManager::_onRemoveAllComplete);
    connect(_stateMachine, &PlanManagerStateMachine::progressChanged, this, &PlanManager::progressPctChanged);
    connect(_stateMachine, &PlanManagerStateMachine::errorOccurred, this, &PlanManager::error);
    connect(_stateMachine, &PlanManagerStateMachine::transactionComplete, this, [this]() {
        emit inProgressChanged(inProgress());
    });
}

PlanManager::~PlanManager()
{
}


void PlanManager::writeMissionItems(const QList<MissionItem*>& missionItems)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (inProgress()) {
        qCDebug(PlanManagerLog) << QStringLiteral("writeMissionItems %1 called while transaction in progress").arg(_planTypeString());
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("writeMissionItems %1 count:").arg(_planTypeString()) << missionItems.count();

    QList<MissionItem*> itemsToWrite;

    bool skipFirstItem = _planType == MAV_MISSION_TYPE_MISSION && !_vehicle->firmwarePlugin()->sendHomePositionToVehicle();

    if (skipFirstItem && missionItems.count() > 0) {
        // First item is not going to be written, free it now.
        delete missionItems[0];
    }

    int firstIndex = skipFirstItem ? 1 : 0;

    for (int i = firstIndex; i < missionItems.count(); i++) {
        MissionItem* item = missionItems[i];
        itemsToWrite.append(item);

        item->setIsCurrentItem(i == firstIndex);

        if (skipFirstItem) {
            // Home is in sequence 0, remainder of items start at sequence 1
            item->setSequenceNumber(item->sequenceNumber() - 1);
            if (item->command() == MAV_CMD_DO_JUMP) {
                item->setParam1(static_cast<int>(item->param1()) - 1);
            }
        }
    }

    _connectToMavlink();
    emit inProgressChanged(true);
    _stateMachine->startWrite(itemsToWrite);
}

void PlanManager::loadFromVehicle(void)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("loadFromVehicle %1 read sequence").arg(_planTypeString());

    if (inProgress()) {
        qCDebug(PlanManagerLog) << QStringLiteral("loadFromVehicle %1 called while transaction in progress").arg(_planTypeString());
        return;
    }

    _connectToMavlink();
    emit inProgressChanged(true);
    _stateMachine->startRead();
}


/// Called when a new mavlink message for out vehicle is received
void PlanManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    _stateMachine->handleMessage(message);
}

void PlanManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(PlanManagerLog) << QStringLiteral("Sending error - _planTypeString(%1) errorCode(%2) errorMsg(%4)").arg(_planTypeString()).arg(errorCode).arg(errorMsg);

    emit error(errorCode, errorMsg);
}


QString PlanManager::_lastMissionReqestString(MAV_MISSION_RESULT result)
{
    QString prefix;
    QString postfix;

    if (_lastMissionRequest >= 0 && _lastMissionRequest < _writeMissionItems.count()) {
        MissionItem* item = _writeMissionItems[_lastMissionRequest];

        prefix = tr("Item #%1 Command: %2").arg(_lastMissionRequest).arg(MissionCommandTree::instance()->friendlyName(item->command()));

        switch (result) {
        case MAV_MISSION_UNSUPPORTED_FRAME:
            postfix = tr("Frame: %1").arg(item->frame());
            break;
        case MAV_MISSION_UNSUPPORTED:
            // All we need is the prefix
            break;
        case MAV_MISSION_INVALID_PARAM1:
            postfix = tr("Value: %1").arg(item->param1());
            break;
        case MAV_MISSION_INVALID_PARAM2:
            postfix = tr("Value: %1").arg(item->param2());
            break;
        case MAV_MISSION_INVALID_PARAM3:
            postfix = tr("Value: %1").arg(item->param3());
            break;
        case MAV_MISSION_INVALID_PARAM4:
            postfix = tr("Value: %1").arg(item->param4());
            break;
        case MAV_MISSION_INVALID_PARAM5_X:
            postfix = tr("Value: %1").arg(item->param5());
            break;
        case MAV_MISSION_INVALID_PARAM6_Y:
            postfix = tr("Value: %1").arg(item->param6());
            break;
        case MAV_MISSION_INVALID_PARAM7:
            postfix = tr("Value: %1").arg(item->param7());
            break;
        case MAV_MISSION_INVALID_SEQUENCE:
            // All we need is the prefix
            break;
        default:
            break;
        }
    }

    return prefix + (postfix.isEmpty() ? QStringLiteral("") : QStringLiteral(" ")) + postfix;
}

QString PlanManager::_missionResultToString(MAV_MISSION_RESULT result)
{
    QString error;

    switch (result) {
    case MAV_MISSION_ACCEPTED:
        error = tr("Mission accepted.");
        break;
    case MAV_MISSION_ERROR:
        error = tr("Unspecified error.");
        break;
    case MAV_MISSION_UNSUPPORTED_FRAME:
        error = tr("Coordinate frame is not supported.");
        break;
    case MAV_MISSION_UNSUPPORTED:
        error = tr("Command is not supported.");
        break;
    case MAV_MISSION_NO_SPACE:
        error = tr("Mission item exceeds storage space.");
        break;
    case MAV_MISSION_INVALID:
        error = tr("One of the parameters has an invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM1:
        error = tr("Param 1 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM2:
        error = tr("Param 2 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM3:
        error = tr("Param 3 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM4:
        error = tr("Param 4 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM5_X:
        error = tr("Param 5 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM6_Y:
        error = tr("Param 6 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM7:
        error = tr("Param 7 invalid value.");
        break;
    case MAV_MISSION_INVALID_SEQUENCE:
        error = tr("Received mission item out of sequence.");
        break;
    case MAV_MISSION_DENIED:
        error = tr("Not accepting any mission commands.");
        break;
    default:
        qWarning(PlanManagerLog) << QStringLiteral("Fell off end of switch statement %1 %2").arg(_planTypeString()).arg(result);
        error = tr("Unknown error: %1.").arg(result);
        break;
    }

    QString lastRequestString = _lastMissionReqestString(result);
    if (!lastRequestString.isEmpty()) {
        error += QStringLiteral(" ") + lastRequestString;
    }

    return error;
}

bool PlanManager::inProgress(void) const
{
    return _stateMachine->inProgress();
}

void PlanManager::_onReadComplete(bool success)
{
    _disconnectFromMavlink();

    if (success) {
        // Transfer items from state machine to our list (take ownership)
        _clearAndDeleteMissionItems();
        _missionItems = _stateMachine->takeMissionItems();
    } else {
        _clearAndDeleteMissionItems();
    }

    emit newMissionItemsAvailable(false);

    if (_resumeMission) {
        _resumeMission = false;
        if (success) {
            emit resumeMissionReady();
        } else {
            emit resumeMissionUploadFail();
        }
    }
}

void PlanManager::_onWriteComplete(bool success)
{
    _disconnectFromMavlink();

    if (success) {
        // Write succeeded, update internal list to be current
        if (_planType == MAV_MISSION_TYPE_MISSION) {
            _currentMissionIndex = -1;
            _lastCurrentIndex = -1;
            emit currentIndexChanged(-1);
            emit lastCurrentIndexChanged(-1);
        }
        _clearAndDeleteMissionItems();
        // Take ownership of written items
        _missionItems = _stateMachine->takeWriteMissionItems();
        _writeMissionItems.clear();
    } else {
        _clearAndDeleteWriteMissionItems();
    }

    emit sendComplete(!success /* error */);

    if (_resumeMission) {
        _resumeMission = false;
        if (success) {
            emit resumeMissionReady();
        } else {
            emit resumeMissionUploadFail();
        }
    }
}

void PlanManager::_onRemoveAllComplete(bool success)
{
    _disconnectFromMavlink();
    emit removeAllComplete(!success /* error */);

    if (_resumeMission) {
        _resumeMission = false;
        if (success) {
            emit resumeMissionReady();
        } else {
            emit resumeMissionUploadFail();
        }
    }
}

void PlanManager::removeAll(void)
{
    if (inProgress()) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("removeAll %1").arg(_planTypeString());

    _clearAndDeleteMissionItems();

    if (_planType == MAV_MISSION_TYPE_MISSION) {
        _currentMissionIndex = -1;
        _lastCurrentIndex = -1;
        emit currentIndexChanged(-1);
        emit lastCurrentIndexChanged(-1);
    }

    _connectToMavlink();
    emit inProgressChanged(true);
    _stateMachine->startRemoveAll();
}

void PlanManager::_clearAndDeleteMissionItems(void)
{
    for (int i=0; i<_missionItems.count(); i++) {
        // Using deleteLater here causes too much transient memory to stack up
        delete _missionItems[i];
    }
    _missionItems.clear();
}


void PlanManager::_clearAndDeleteWriteMissionItems(void)
{
    for (int i=0; i<_writeMissionItems.count(); i++) {
        // Using deleteLater here causes too much transient memory to stack up
        delete _writeMissionItems[i];
    }
    _writeMissionItems.clear();
}

void PlanManager::_connectToMavlink(void)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &PlanManager::_mavlinkMessageReceived);
}

void PlanManager::_disconnectFromMavlink(void)
{
    disconnect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &PlanManager::_mavlinkMessageReceived);
}

QString PlanManager::_planTypeString(void)
{
    switch (_planType) {
    case MAV_MISSION_TYPE_MISSION:
        return QStringLiteral("T:Mission");
    case MAV_MISSION_TYPE_FENCE:
        return QStringLiteral("T:GeoFence");
    case MAV_MISSION_TYPE_RALLY:
        return QStringLiteral("T:Rally");
    default:
        qWarning() << "Unknown plan type" << _planType;
        return QStringLiteral("T:Unknown");
    }
}

