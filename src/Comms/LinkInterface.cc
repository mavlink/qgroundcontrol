/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "AppSettings.h"

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(LinkInterfaceLog, "LinkInterfaceLog")

LinkInterface::LinkInterface(SharedLinkConfigurationPtr &config, bool isPX4Flow, QObject *parent)
    : QThread(parent)
    , m_config(config)
    , m_isPX4Flow(isPX4Flow)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

LinkInterface::~LinkInterface()
{
    if (m_vehicleReferenceCount != 0) {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "still have vehicle references:" << m_vehicleReferenceCount;
    }

    m_config.reset();
}

uint8_t LinkInterface::mavlinkChannel() const
{
    if (!mavlinkChannelIsSet()) {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "mavlinkChannelIsSet() == false";
    }

    return m_mavlinkChannel;
}

bool LinkInterface::mavlinkChannelIsSet() const
{
    return (LinkManager::invalidMavlinkChannel() != m_mavlinkChannel);
}

bool LinkInterface::initMavlinkSigning(void)
{
    if (!isSecureConnection()) {
        auto appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
        QByteArray signingKeyBytes = appSettings->mavlink2SigningKey()->rawValue().toByteArray();
        if (MAVLinkSigning::initSigning(static_cast<mavlink_channel_t>(m_mavlinkChannel), signingKeyBytes, MAVLinkSigning::insecureConnectionAccceptUnsignedCallback)) {
            if (signingKeyBytes.isEmpty()) {
                qCDebug(LinkInterfaceLog) << "Signing disabled on channel" << m_mavlinkChannel;
            } else {
                qCDebug(LinkInterfaceLog) << "Signing enabled on channel" << m_mavlinkChannel;
            }
        } else {
            qWarning() << Q_FUNC_INFO << "Failed To enable Signing on channel" << m_mavlinkChannel;
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
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "already have" << m_mavlinkChannel;
        return true;
    }

    m_mavlinkChannel = qgcApp()->toolbox()->linkManager()->allocateMavlinkChannel();

    if (!mavlinkChannelIsSet()) {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "failed";
        return false;
    }

    qCDebug(LinkInterfaceLog) << "_allocateMavlinkChannel" << m_mavlinkChannel;

    initMavlinkSigning();

    return true;
}

void LinkInterface::_freeMavlinkChannel()
{
    qCDebug(LinkInterfaceLog) << Q_FUNC_INFO << m_mavlinkChannel;

    if (!mavlinkChannelIsSet()) {
        return;
    }

    qgcApp()->toolbox()->linkManager()->freeMavlinkChannel(m_mavlinkChannel);
    m_mavlinkChannel = LinkManager::invalidMavlinkChannel();
}

void LinkInterface::writeBytesThreadSafe(const char *bytes, int length)
{
    const QByteArray data(bytes, length);
    (void) QMetaObject::invokeMethod(this, "_writeBytes", Qt::AutoConnection, data);
}

void LinkInterface::removeVehicleReference()
{
    if (m_vehicleReferenceCount != 0) {
        m_vehicleReferenceCount--;
        _connectionRemoved();
    } else {
        qCWarning(LinkInterfaceLog) << Q_FUNC_INFO << "called with no vehicle references";
    }
}

void LinkInterface::_connectionRemoved()
{
    if (m_vehicleReferenceCount == 0) {
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
