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

QGC_LOGGING_CATEGORY(LinkInterfaceLog, "LinkInterfaceLog")

bool LinkInterface::Channel::reserve(LinkManager& mgr)
{
    if (_set) {
        qCWarning(LinkInterfaceLog) << "Channel set multiple times";
    }
    uint8_t channel = mgr.reserveMavlinkChannel();
    if (0 == channel) {
        qCWarning(LinkInterfaceLog) << "Channel reserve failed";
        return false;
    }
    qCDebug(LinkInterfaceLog) << "Channel::reserve" << channel;
    _set = true;
    _id = channel;
    return true;
}

void LinkInterface::Channel::free(LinkManager& mgr)
{
    qCDebug(LinkInterfaceLog) << "Channel::free" << _id;
    if (0 != _id) {
        mgr.freeMavlinkChannel(_id);
    }
    _set = false;
    _id = 0;
}

uint8_t LinkInterface::Channel::id(void) const
{
    if (!_set) {
        qCWarning(LinkInterfaceLog) << "Call to LinkInterface::mavlinkChannel with _mavlinkChannel.set() == false";
    }
    return _id;
}

LinkInterface::LinkInterface(SharedLinkConfigurationPtr& config, bool isPX4Flow)
    : QThread   (0)
    , _config   (config)
    , _isPX4Flow(isPX4Flow)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    qRegisterMetaType<LinkInterface*>("LinkInterface*");
}

LinkInterface::~LinkInterface()
{
    if (_vehicleReferenceCount != 0) {
        qCWarning(LinkInterfaceLog) << "~LinkInterface still have vehicle references:" << _vehicleReferenceCount;
    }
    _config.reset();
}

uint8_t LinkInterface::mavlinkChannel(void) const
{
    return _mavlinkChannel.id();
}

bool LinkInterface::_reserveMavlinkChannel(LinkManager& mgr)
{
    return _mavlinkChannel.reserve(mgr);
}

void LinkInterface::_freeMavlinkChannel(LinkManager& mgr)
{
    return _mavlinkChannel.free(mgr);
}

void LinkInterface::writeBytesThreadSafe(const char *bytes, int length)
{
    QByteArray byteArray(bytes, length);
    _writeBytesMutex.lock();
    _writeBytes(byteArray);
    _writeBytesMutex.unlock();
}

void LinkInterface::addVehicleReference(void)
{
    _vehicleReferenceCount++;
}

void LinkInterface::removeVehicleReference(void)
{
    if (_vehicleReferenceCount != 0) {
        _vehicleReferenceCount--;
        if (_vehicleReferenceCount == 0) {
            disconnect();
        }
    } else {
        qCWarning(LinkInterfaceLog) << "LinkInterface::removeVehicleReference called with no vehicle references";
    }
}

void LinkInterface::_connectionRemoved(void)
{
    if (_vehicleReferenceCount == 0) {
        // Since there are no vehicles on the link we can disconnect it right now
        disconnect();
    } else {
        // If there are still vehicles on this link we allow communication lost to trigger and don't automatically disconect until all the vehicles go away
    }
}

#ifdef UNITTEST_BUILD
#include "MockLink.h"
bool LinkInterface::isMockLink(void)
{
    return dynamic_cast<MockLink*>(this);
}
#endif
