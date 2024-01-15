/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleLinkManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"
#include "LinkManager.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(VehicleLinkManagerLog, "VehicleLinkManagerLog")

VehicleLinkManager::VehicleLinkManager(Vehicle* vehicle)
    : QObject   (vehicle)
    , _vehicle  (vehicle)
    , _linkMgr  (qgcApp()->toolbox()->linkManager())
{
    connect(this,                   &VehicleLinkManager::linkNamesChanged,  this, &VehicleLinkManager::linkStatusesChanged);
    connect(&_commLostCheckTimer,   &QTimer::timeout,                       this, &VehicleLinkManager::_commLostCheck);

    _commLostCheckTimer.setSingleShot(false);
    _commLostCheckTimer.setInterval(_commLostCheckTimeoutMSecs);
}

void VehicleLinkManager::mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    // Radio status messages come from Sik Radios directly. It doesn't indicate there is any life on the other end.
    if (message.msgid != MAVLINK_MSG_ID_RADIO_STATUS) {
        int linkIndex = _containsLinkIndex(link);
        if (linkIndex == -1) {
            _addLink(link);
        } else {
            LinkInfo_t& linkInfo = _rgLinkInfo[linkIndex];
            linkInfo.heartbeatElapsedTimer.restart();
            if (_rgLinkInfo[linkIndex].commLost) {
                _commRegainedOnLink(link);
            }
        }
    }
}

void VehicleLinkManager::_commRegainedOnLink(LinkInterface* link)
{
    QString commRegainedMessage;
    QString primarySwitchMessage;

    int linkIndex = _containsLinkIndex(link);
    if (linkIndex == -1) {
        return;
    }

    _rgLinkInfo[linkIndex].commLost = false;

    // Notify the user of communication regained
    bool isPrimaryLink = link == _primaryLink.lock().get();
    if (_rgLinkInfo.count() > 1) {
        commRegainedMessage = tr("%1Communication regained on %2 link").arg(_vehicle->_vehicleIdSpeech()).arg(isPrimaryLink ? tr("primary") : tr("secondary"));
    } else {
        commRegainedMessage = tr("%1Communication regained").arg(_vehicle->_vehicleIdSpeech());
    }

    // Try to switch to another link
    if (_updatePrimaryLink()) {
        QString primarySwitchMessage = tr("%1Switching communication to new primary link").arg(_vehicle->_vehicleIdSpeech());
    }

    if (!commRegainedMessage.isEmpty()) {
        _vehicle->_say(commRegainedMessage);
    }
    if (!primarySwitchMessage.isEmpty()) {
        _vehicle->_say(primarySwitchMessage);
        qgcApp()->showAppMessage(primarySwitchMessage);
    }

    emit linkStatusesChanged();

    // Check recovery from total communication loss
    if (_communicationLost) {
        bool noCommunicationLoss = true;
        for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
            if (linkInfo.commLost) {
                noCommunicationLoss = false;
                break;
            }
        }
        if (noCommunicationLoss) {
            _communicationLost = false;
            emit communicationLostChanged(false);
        }
    }
}

void VehicleLinkManager::_commLostCheck(void)
{
    QString switchingPrimaryLinkMessage;

    if (!_communicationLostEnabled) {
        return;
    }

    bool linkStatusChange = false;
    for (LinkInfo_t& linkInfo: _rgLinkInfo) {
        if (!linkInfo.commLost && !linkInfo.link->linkConfiguration()->isHighLatency() && linkInfo.heartbeatElapsedTimer.elapsed() > _heartbeatMaxElpasedMSecs) {
            linkInfo.commLost = true;
            linkStatusChange = true;

            // Notify the user of individual link communication loss
            bool isPrimaryLink = linkInfo.link.get() == _primaryLink.lock().get();
            if (_rgLinkInfo.count() > 1) {
                QString msg = tr("%1Communication lost on %2 link.").arg(_vehicle->_vehicleIdSpeech()).arg(isPrimaryLink ? tr("primary") : tr("secondary"));
                _vehicle->_say(msg);
            }
        }
    }
    if (linkStatusChange) {
        emit linkStatusesChanged();
    }

    // Switch to better primary link if needed
    if (_updatePrimaryLink()) {
        QString msg = tr("%1Switching communication to secondary link.").arg(_vehicle->_vehicleIdSpeech());
        _vehicle->_say(msg);
        qgcApp()->showAppMessage(msg);
    }

    // Check for total communication loss
    if (!_communicationLost) {
        bool totalCommunicationLoss = true;
        for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
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
            _vehicle->_say(tr("%1Communication lost").arg(_vehicle->_vehicleIdSpeech()));

            _communicationLost = true;
            emit communicationLostChanged(true);
        }
    }
}

int VehicleLinkManager::_containsLinkIndex(LinkInterface* link)
{
    for (int i=0; i<_rgLinkInfo.count(); i++) {
        if (_rgLinkInfo[i].link.get() == link) {
            return i;
        }
    }
    return -1;
}

void VehicleLinkManager::_addLink(LinkInterface* link)
{
    if (_containsLinkIndex(link) != -1) {
        qCWarning(VehicleLinkManagerLog) << "_addLink call with link which is already in the list";
        return;
    } else {
        SharedLinkInterfacePtr sharedLink = _linkMgr->sharedLinkInterfacePointerForLink(link);
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

        connect(link, &LinkInterface::disconnected, this, &VehicleLinkManager::_linkDisconnected);

        emit linkNamesChanged();

        if (_rgLinkInfo.count() == 1) {
            _commLostCheckTimer.start();
        }
    }
}

void VehicleLinkManager::_removeLink(LinkInterface* link)
{
    int linkIndex = _containsLinkIndex(link);

    if (linkIndex == -1) {
        qCWarning(VehicleLinkManagerLog) << "_removeLink call with link which is already in the list";
        return;
    } else {
        qCDebug(VehicleLinkManagerLog) << "_removeLink:" << QString("%1").arg((qulonglong)link, 0, 16);

        if (link == _primaryLink.lock().get()) {
            _primaryLink.reset();
            emit primaryLinkChanged();
        }

        disconnect(link, &LinkInterface::disconnected, this, &VehicleLinkManager::_linkDisconnected);
        link->removeVehicleReference();
        emit linkNamesChanged();
        _rgLinkInfo.removeAt(linkIndex); // Remove the link last since it may cause the link itself to be deleted

        if (_rgLinkInfo.count() == 0) {
            _commLostCheckTimer.stop();
        }
    }
}

void VehicleLinkManager::_linkDisconnected(void)
{
    qCDebug(VehicleLog) << "_linkDisconnected linkCount" << _rgLinkInfo.count();

    LinkInterface* link = qobject_cast<LinkInterface*>(sender());
    if (link) {
        _removeLink(link);
        _updatePrimaryLink();
        if (_rgLinkInfo.count() == 0) {
            qCDebug(VehicleLog) << "All links removed. Closing down Vehicle.";
            emit allLinksRemoved(_vehicle);
        }
    }
}

SharedLinkInterfacePtr VehicleLinkManager::_bestActivePrimaryLink(void)
{
#ifndef NO_SERIAL_LINK
    // Best choice is a USB connection
    for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
        if (!linkInfo.commLost) {
            SharedLinkInterfacePtr  link        = linkInfo.link;
            SerialLink*             serialLink  = qobject_cast<SerialLink*>(link.get());
            if (serialLink) {
                SharedLinkConfigurationPtr config = serialLink->linkConfiguration();
                if (config) {
                    SerialConfiguration* serialConfig = qobject_cast<SerialConfiguration*>(config.get());
                    if (serialConfig && serialConfig->usbDirect()) {
                        return link;
                    }
                }
            }
        }
    }
#endif

    // Next best is normal latency link
    for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
        if (!linkInfo.commLost) {
            SharedLinkInterfacePtr      link    = linkInfo.link;
            SharedLinkConfigurationPtr  config  = link->linkConfiguration();
            if (config && !config->isHighLatency()) {
                return link;
            }
        }
    }

    // Last possible choice is a high latency link
    SharedLinkInterfacePtr link = _primaryLink.lock();
    if (link && link->linkConfiguration()->isHighLatency()) {
        // Best choice continues to be the current high latency link
        return link;
    } else {
        // Pick any high latency link if one exists
        for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
            if (!linkInfo.commLost) {
                SharedLinkInterfacePtr      link    = linkInfo.link;
                SharedLinkConfigurationPtr  config  = link->linkConfiguration();
                if (config && config->isHighLatency()) {
                    return link;
                }
            }
        }
    }

    return {};
}

bool VehicleLinkManager::_updatePrimaryLink(void)
{
    SharedLinkInterfacePtr primaryLink = _primaryLink.lock();
    int linkIndex = _containsLinkIndex(primaryLink.get());
    if (linkIndex != -1 && !_rgLinkInfo[linkIndex].commLost && !primaryLink->linkConfiguration()->isHighLatency()) {
        // Current priority link is still valid
        return false;
    }

    SharedLinkInterfacePtr bestActivePrimaryLink = _bestActivePrimaryLink();

    if (linkIndex != -1 && !bestActivePrimaryLink) {
        // Nothing better available, leave things set to current primary link
        return false;
    } else {
        if (bestActivePrimaryLink != primaryLink) {
            if (primaryLink && primaryLink->linkConfiguration()->isHighLatency()) {
                _vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                               MAV_CMD_CONTROL_HIGH_LATENCY,
                               true,
                               0); // Stop transmission on this link
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
        } else {
            return false;
        }
    }
}

void VehicleLinkManager::closeVehicle(void)
{
    // Vehicle is no longer communicating with us. Remove all link references

    QList<LinkInfo_t> rgLinkInfoCopy = _rgLinkInfo;
    for (const LinkInfo_t& linkInfo: rgLinkInfoCopy) {
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
    return _containsLinkIndex(link) != -1;
}

QString VehicleLinkManager::primaryLinkName() const
{
    if (!_primaryLink.expired()) {
        return _primaryLink.lock()->linkConfiguration()->name();
    }

    return QString();
}
void VehicleLinkManager::setPrimaryLinkByName(const QString& name)
{
    for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
        if (linkInfo.link->linkConfiguration()->name() == name) {
            _primaryLink = linkInfo.link;
            emit primaryLinkChanged();
        }
    }
}

QStringList VehicleLinkManager::linkNames(void) const
{
    QStringList rgNames;

    for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
        rgNames.append(linkInfo.link->linkConfiguration()->name());
    }

    return rgNames;
}

QStringList VehicleLinkManager::linkStatuses(void) const
{
    QStringList rgStatuses;

    for (const LinkInfo_t& linkInfo: _rgLinkInfo) {
        rgStatuses.append(linkInfo.commLost ? tr("Comm Lost") : "");
    }

    return rgStatuses;
}

bool VehicleLinkManager::primaryLinkIsPX4Flow(void) const
{
    SharedLinkInterfacePtr sharedLink = _primaryLink.lock();
    if (!sharedLink) {
        return false;
    } else {
        return sharedLink->isPX4Flow();
    }
}
