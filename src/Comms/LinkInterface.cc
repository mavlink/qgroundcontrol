/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LinkInterface.h"
#include "LinkManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "MAVLinkSigning.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(LinkInterfaceLog, "qgc.comms.linkinterface")

LinkInterface::LinkInterface(SharedLinkConfigurationPtr &config, QObject *parent)
    : QObject(parent)
    , _config(config)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

LinkInterface::~LinkInterface()
{
    if (_vehicleReferenceCount != 0) {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "still have vehicle references:" << _vehicleReferenceCount;
    }

    _config.reset();
}

uint8_t LinkInterface::mavlinkChannel() const
{
    if (!mavlinkChannelIsSet()) {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "mavlinkChannelIsSet() == false";
    }

    return _mavlinkChannel;
}

bool LinkInterface::mavlinkChannelIsSet() const
{
    return (LinkManager::invalidMavlinkChannel() != _mavlinkChannel);
}

bool LinkInterface::initMavlinkSigning()
{
    if (!isSecureConnection()) {
        auto mavlinkSettings = SettingsManager::instance()->mavlinkSettings();
        const QByteArray signingKeyBytes = mavlinkSettings->mavlink2SigningKey()->rawValue().toByteArray();
        if (MAVLinkSigning::initSigning(static_cast<mavlink_channel_t>(_mavlinkChannel), signingKeyBytes, MAVLinkSigning::insecureConnectionAccceptUnsignedCallback)) {
            if (signingKeyBytes.isEmpty()) {
                qCDebug(LinkInterfaceLog) << "Signing disabled on channel" << _mavlinkChannel;
            } else {
                qCDebug(LinkInterfaceLog) << "Signing enabled on channel" << _mavlinkChannel;
            }
        } else {
            qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "Failed To enable Signing on channel" << _mavlinkChannel;
            // FIXME: What should we do here?
            return false;
        }
    }

    return true;
}

bool LinkInterface::_allocateMavlinkChannel()
{
    Q_ASSERT(!mavlinkChannelIsSet());

    if (mavlinkChannelIsSet()) {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "already have" << _mavlinkChannel;
        return true;
    }

    _mavlinkChannel = LinkManager::instance()->allocateMavlinkChannel();

    if (!mavlinkChannelIsSet()) {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "failed";
        return false;
    }

    qCDebug(LinkInterfaceLog) << "_allocateMavlinkChannel" << _mavlinkChannel;

    initMavlinkSigning();

    return true;
}

void LinkInterface::_freeMavlinkChannel()
{
    qCDebug(LinkInterfaceLog) << Q_FUNC_INFO << _mavlinkChannel;

    if (!mavlinkChannelIsSet()) {
        return;
    }

    LinkManager::instance()->freeMavlinkChannel(_mavlinkChannel);
    _mavlinkChannel = LinkManager::invalidMavlinkChannel();
}

void LinkInterface::writeBytesThreadSafe(const char *bytes, int length)
{
    const QByteArray data(bytes, length);
    (void) QMetaObject::invokeMethod(this, "_writeBytes", Qt::AutoConnection, data);
}

void LinkInterface::removeVehicleReference()
{
    if (_vehicleReferenceCount != 0) {
        _vehicleReferenceCount--;
        _connectionRemoved();
    } else {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "called with no vehicle references";
    }
}

void LinkInterface::_connectionRemoved()
{
    if (_vehicleReferenceCount == 0) {
        // Since there are no vehicles on the link we can disconnect it right now
        disconnect();
    } else {
        // If there are still vehicles on this link we allow communication lost to trigger and don't automatically disconect until all the vehicles go away
    }
}

void LinkInterface::setSigningSignatureFailure(bool failure)
{
    if (_signingSignatureFailure != failure) {
        _signingSignatureFailure = failure;
        if (_signingSignatureFailure) {
            emit communicationError(tr("Signing Failure"), tr("Signing signature mismatch"));
        }
    }
}
