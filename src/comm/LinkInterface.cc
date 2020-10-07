/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LinkInterface.h"
#include "QGCApplication.h"

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
        qWarning() << "~LinkInterface still have vehicle references:" << _vehicleReferenceCount;
    }
    _config.reset();
}

uint8_t LinkInterface::mavlinkChannel(void) const
{
    if (!_mavlinkChannelSet) {
        qWarning() << "Call to LinkInterface::mavlinkChannel with _mavlinkChannelSet == false";
    }
    return _mavlinkChannel;
}

void LinkInterface::_setMavlinkChannel(uint8_t channel)
{
    if (_mavlinkChannelSet) {
        qWarning() << "Mavlink channel set multiple times";
    }
    _mavlinkChannelSet = true;
    _mavlinkChannel = channel;
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
        qWarning() << "LinkInterface::removeVehicleReference called with no vehicle references";
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
