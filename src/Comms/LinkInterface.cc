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

    qCDebug(LinkInterfaceLog) << Q_FUNC_INFO << m_mavlinkChannel;
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
