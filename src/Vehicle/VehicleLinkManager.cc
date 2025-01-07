/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleLinkManager.h"
#include "Vehicle.h"
#include "LinkManager.h"
#include "QGCApplication.h"
#include "AudioOutput.h"
#ifndef QGC_NO_SERIAL_LINK
    #include "SerialLink.h"
#endif
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VehicleLinkManagerLog, "qgc.vehicle.vehiclelinkmanager")

VehicleLinkManager::VehicleLinkManager(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _commLostCheckTimer(new QTimer(this))
{
    // qCDebug(VehicleLinkManagerLog) << Q_FUNC_INFO << this;

    (void) connect(this, &VehicleLinkManager::linkNamesChanged, this, &VehicleLinkManager::linkStatusesChanged);
    (void) connect(_commLostCheckTimer, &QTimer::timeout, this, &VehicleLinkManager::_commLostCheck);

    _commLostCheckTimer->setSingleShot(false);
    _commLostCheckTimer->setInterval(_commLostCheckTimeoutMSecs);
}

VehicleLinkManager::~VehicleLinkManager()
{
    // qCDebug(VehicleLinkManagerLog) << Q_FUNC_INFO << this;
}

void VehicleLinkManager::mavlinkMessageReceived(LinkInterface *link, const mavlink_message_t &message)
{
    // Radio status messages come from Sik Radios directly. It doesn't indicate there is any life on the other end.
    if (message.msgid == MAVLINK_MSG_ID_RADIO_STATUS) {
        return;
    }

    const int linkIndex = _containsLinkIndex(link);
    if (linkIndex == -1) {
        _addLink(link);
        return;
    }

    LinkInfo_t &linkInfo = _rgLinkInfo[linkIndex];
    linkInfo.heartbeatElapsedTimer.restart();
    if (_rgLinkInfo[linkIndex].commLost) {
        _commRegainedOnLink(link);
    }
}

void VehicleLinkManager::_commRegainedOnLink(LinkInterface *link)
{

    const int linkIndex = _containsLinkIndex(link);
    if (linkIndex == -1) {
        return;
    }

    _rgLinkInfo[linkIndex].commLost = false;

    // Notify the user of communication regained
    QString commRegainedMessage;
    const bool isPrimaryLink = link == _primaryLink.lock().get();
    if (_rgLinkInfo.count() > 1) {
        commRegainedMessage = tr("%1Communication regained on %2 link").arg(_vehicle->_vehicleIdSpeech()).arg(isPrimaryLink ? tr("primary") : tr("secondary"));
    } else {
        commRegainedMessage = tr("%1Communication regained").arg(_vehicle->_vehicleIdSpeech());
    }

    // Try to switch to another link
    QString primarySwitchMessage;
    if (_updatePrimaryLink()) {
        primarySwitchMessage = tr("%1Switching communication to new primary link").arg(_vehicle->_vehicleIdSpeech());
    }

    if (!commRegainedMessage.isEmpty()) {
        AudioOutput::instance()->say(commRegainedMessage.toLower());
    }

    if (!primarySwitchMessage.isEmpty()) {
        AudioOutput::instance()->say(primarySwitchMessage.toLower());
        qgcApp()->showAppMessage(primarySwitchMessage);
    }

    emit linkStatusesChanged();

    // Check recovery from total communication loss
    if (!_communicationLost) {
        return;
    }

    bool noCommunicationLoss = true;
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            noCommunicationLoss = false;
            break;
        }
    }

    if (noCommunicationLoss) {
        _communicationLost = false;
        emit communicationLostChanged(_communicationLost);
    }
}

void VehicleLinkManager::_commLostCheck()
{
    if (!_communicationLostEnabled) {
        return;
    }

    bool linkStatusChange = false;
    for (LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (!linkInfo.commLost && !linkInfo.link->linkConfiguration()->isHighLatency() && (linkInfo.heartbeatElapsedTimer.elapsed() > _heartbeatMaxElpasedMSecs)) {
            linkInfo.commLost = true;
            linkStatusChange = true;

            // Notify the user of individual link communication loss
            const bool isPrimaryLink = linkInfo.link.get() == _primaryLink.lock().get();
            if (_rgLinkInfo.count() > 1) {
                const QString msg = tr("%1Communication lost on %2 link.").arg(_vehicle->_vehicleIdSpeech()).arg(isPrimaryLink ? tr("primary") : tr("secondary"));
                AudioOutput::instance()->say(msg.toLower());
            }
        }
    }

    if (linkStatusChange) {
        emit linkStatusesChanged();
    }

    if (_updatePrimaryLink()) {
        QString msg = tr("%1Switching communication to secondary link.").arg(_vehicle->_vehicleIdSpeech());
        AudioOutput::instance()->say(msg.toLower());
        qgcApp()->showAppMessage(msg);
    }

    if (_communicationLost) {
        return;
    }

    bool totalCommunicationLoss = true;
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (!linkInfo.commLost) {
            totalCommunicationLoss = false;
            break;
        }
    }

    if (totalCommunicationLoss) {
        if (_autoDisconnect) {
            // There is only one link to the vehicle and we want to auto disconnect from it
            closeVehicle();
            return;
        }

        AudioOutput::instance()->say(tr("%1Communication lost").arg(_vehicle->_vehicleIdSpeech()).toLower());

        _communicationLost = true;
        emit communicationLostChanged(_communicationLost);
    }
}

int VehicleLinkManager::_containsLinkIndex(const LinkInterface *link)
{
    for (int i = 0; i < _rgLinkInfo.count(); i++) {
        if (_rgLinkInfo[i].link.get() == link) {
            return i;
        }
    }

    return -1;
}

void VehicleLinkManager::_addLink(LinkInterface *link)
{
    if (_containsLinkIndex(link) != -1) {
        qCWarning(VehicleLinkManagerLog) << "_addLink call with link which is already in the list";
        return;
    }

    SharedLinkInterfacePtr sharedLink = LinkManager::instance()->sharedLinkInterfacePointerForLink(link);
    if (!sharedLink) {
        qCDebug(VehicleLinkManagerLog) << "_addLink stale link" << (void*)link;
        return;
    }

    qCDebug(VehicleLinkManagerLog) << "_addLink:" << link->linkConfiguration()->name() << QString("%1").arg((qulonglong)link, 0, 16);

    link->addVehicleReference();

    LinkInfo_t linkInfo;
    linkInfo.link = sharedLink;
    if (!link->linkConfiguration()->isHighLatency()) {
        linkInfo.heartbeatElapsedTimer.start();
    }
    _rgLinkInfo.append(linkInfo);

    _updatePrimaryLink();

    (void) connect(link, &LinkInterface::disconnected, this, &VehicleLinkManager::_linkDisconnected);

    emit linkNamesChanged();

    if (_rgLinkInfo.count() == 1) {
        _commLostCheckTimer->start();
    }
}

void VehicleLinkManager::_removeLink(LinkInterface *link)
{
    const int linkIndex = _containsLinkIndex(link);
    if (linkIndex == -1) {
        qCWarning(VehicleLinkManagerLog) << "_removeLink call with link which is already in the list";
        return;
    }

    qCDebug(VehicleLinkManagerLog) << "_removeLink:" << QString("%1").arg((qulonglong)link, 0, 16);

    if (link == _primaryLink.lock().get()) {
        _primaryLink.reset();
        emit primaryLinkChanged();
    }

    disconnect(link, &LinkInterface::disconnected, this, &VehicleLinkManager::_linkDisconnected);
    link->removeVehicleReference();
    emit linkNamesChanged();
    _rgLinkInfo.removeAt(linkIndex); // Remove the link last since it may cause the link itself to be deleted

    if (_rgLinkInfo.isEmpty()) {
        _commLostCheckTimer->stop();
    }
}

void VehicleLinkManager::_linkDisconnected()
{
    qCDebug(VehicleLog) << Q_FUNC_INFO << "linkCount" << _rgLinkInfo.count();

    LinkInterface *link = qobject_cast<LinkInterface*>(sender());
    if (!link) {
        return;
    }

    _removeLink(link);
    _updatePrimaryLink();
    if (_rgLinkInfo.isEmpty()) {
        qCDebug(VehicleLog) << "All links removed. Closing down Vehicle.";
        emit allLinksRemoved(_vehicle);
    }
}

SharedLinkInterfacePtr VehicleLinkManager::_bestActivePrimaryLink()
{
#ifndef QGC_NO_SERIAL_LINK
    // Best choice is a USB connection
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            continue;
        }

        SharedLinkInterfacePtr link = linkInfo.link;
        auto linkInterface = link.get();
        if (linkInterface && LinkManager::isLinkUSBDirect(linkInterface)) {
            return link;
        }
    }
#endif

    // Next best is normal latency link
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            continue;
        }

        SharedLinkInterfacePtr link = linkInfo.link;
        const SharedLinkConfigurationPtr config = link->linkConfiguration();
        if (config && !config->isHighLatency()) {
            return link;
        }
    }

    // Last possible choice is a high latency link
    SharedLinkInterfacePtr link = _primaryLink.lock();
    if (link && link->linkConfiguration()->isHighLatency()) {
        // Best choice continues to be the current high latency link
        return link;
    }

    // Pick any high latency link if one exists
    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        if (linkInfo.commLost) {
            continue;
        }

        SharedLinkInterfacePtr link = linkInfo.link;
        const SharedLinkConfigurationPtr config = link->linkConfiguration();
        if (config && config->isHighLatency()) {
            return link;
        }
    }

    return {};
}

bool VehicleLinkManager::_updatePrimaryLink()
{
    SharedLinkInterfacePtr primaryLink = _primaryLink.lock();
    const int linkIndex = _containsLinkIndex(primaryLink.get());

    if ((linkIndex != -1) && !_rgLinkInfo[linkIndex].commLost && !primaryLink->linkConfiguration()->isHighLatency()) {
        // Current priority link is still valid
        return false;
    }

    SharedLinkInterfacePtr bestActivePrimaryLink = _bestActivePrimaryLink();
    if ((linkIndex != -1) && !bestActivePrimaryLink) {
        // Nothing better available, leave things set to current primary link
        return false;
    }

    if (bestActivePrimaryLink == primaryLink) {
        return false;
    }

    if (primaryLink && primaryLink->linkConfiguration()->isHighLatency()) {
        _vehicle->sendMavCommand(
            MAV_COMP_ID_AUTOPILOT1,
            MAV_CMD_CONTROL_HIGH_LATENCY,
            true,
            0 // Stop transmission on this link
        );
    }

    _primaryLink = bestActivePrimaryLink;
    emit primaryLinkChanged();

    if (bestActivePrimaryLink && bestActivePrimaryLink->linkConfiguration()->isHighLatency()) {
        _vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                       MAV_CMD_CONTROL_HIGH_LATENCY,
                       true,
                       1); // Start transmission on this link
    }

    return true;
}

void VehicleLinkManager::closeVehicle()
{
    // Vehicle is no longer communicating with us. Remove all link references

    const QList<LinkInfo_t> rgLinkInfoCopy = _rgLinkInfo;
    for (const LinkInfo_t &linkInfo: rgLinkInfoCopy) {
        _removeLink(linkInfo.link.get());
    }

    _rgLinkInfo.clear();

    emit allLinksRemoved(_vehicle);
}

void VehicleLinkManager::setCommunicationLostEnabled(bool communicationLostEnabled)
{
    if (_communicationLostEnabled != communicationLostEnabled) {
        _communicationLostEnabled = communicationLostEnabled;
        emit communicationLostEnabledChanged(communicationLostEnabled);
    }
}

bool VehicleLinkManager::containsLink(LinkInterface* link)
{
    return (_containsLinkIndex(link) != -1);
}

QString VehicleLinkManager::primaryLinkName() const
{
    if (!_primaryLink.expired()) {
        return _primaryLink.lock()->linkConfiguration()->name();
    }

    return QString();
}

void VehicleLinkManager::setPrimaryLinkByName(const QString &name)
{
    for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
        if (linkInfo.link->linkConfiguration()->name() == name) {
            _primaryLink = linkInfo.link;
            emit primaryLinkChanged();
        }
    }
}

QStringList VehicleLinkManager::linkNames() const
{
    QStringList rgNames;

    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        rgNames.append(linkInfo.link->linkConfiguration()->name());
    }

    return rgNames;
}

QStringList VehicleLinkManager::linkStatuses() const
{
    QStringList rgStatuses;

    for (const LinkInfo_t &linkInfo: _rgLinkInfo) {
        rgStatuses.append(linkInfo.commLost ? tr("Comm Lost") : "");
    }

    return rgStatuses;
}
