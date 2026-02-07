#include "LinkInterface.h"
#include "LinkManager.h"
#include "QGCApplication.h"
#include <QtCore/QLoggingCategory>
#include "MAVLinkSigning.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"

#include <QtQml/QQmlEngine>

Q_STATIC_LOGGING_CATEGORY(LinkInterfaceLog, "Comms.LinkInterface")

LinkInterface::LinkInterface(SharedLinkConfigurationPtr &config, QObject *parent)
    : QObject(parent)
    , _config(config)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

LinkInterface::~LinkInterface()
{
    if (_vehicleReferenceCount != 0) {
        qCWarning(LinkInterfaceLog) << "still have vehicle references:" << _vehicleReferenceCount;
    }

    _config.reset();
}

uint8_t LinkInterface::mavlinkChannel() const
{
    if (!mavlinkChannelIsSet()) {
        qCWarning(LinkInterfaceLog) << "mavlinkChannelIsSet() == false";
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
            qCWarning(LinkInterfaceLog) << "Failed To enable Signing on channel" << _mavlinkChannel;
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
        qCWarning(LinkInterfaceLog) << "already have" << _mavlinkChannel;
        return true;
    }

    _mavlinkChannel = LinkManager::instance()->allocateMavlinkChannel();

    if (!mavlinkChannelIsSet()) {
        qCWarning(LinkInterfaceLog) << "failed";
        return false;
    }

    qCDebug(LinkInterfaceLog) << "_allocateMavlinkChannel" << _mavlinkChannel;

    mavlink_set_proto_version(_mavlinkChannel, MAVLINK_VERSION); // We only support v2 protcol
    initMavlinkSigning();

    return true;
}

void LinkInterface::_freeMavlinkChannel()
{
    qCDebug(LinkInterfaceLog) << _mavlinkChannel;

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
        qCWarning(LinkInterfaceLog) << "called with no vehicle references";
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

void LinkInterface::reportMavlinkV1Traffic()
{
    if (!_mavlinkV1TrafficReported) {
        _mavlinkV1TrafficReported = true;

        const SharedLinkConfigurationPtr linkConfig = linkConfiguration();
        const QString linkName = linkConfig ? linkConfig->name() : QStringLiteral("unknown");
        qCWarning(LinkInterfaceLog) << "MAVLink v1 traffic detected on link" << linkName;
        const QString message = tr("MAVLink v1 traffic detected on link '%1'. "
                                   "%2 only supports MAVLink v2. "
                                   "Please ensure your vehicle is configured to use MAVLink v2.")
                                    .arg(linkName).arg(qgcApp()->applicationName());
        qgcApp()->showAppMessage(message);
    }
}
