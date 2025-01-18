/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LinkConfiguration.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialLink.h"
#endif
#include "UDPLink.h"
#include "TCPLink.h"
#include "LogReplayLink.h"
#ifdef QGC_ENABLE_BLUETOOTH
#include "BluetoothLink.h"
#endif
#ifdef QT_DEBUG
#include "MockLink.h"
#endif
#ifndef QGC_AIRLINK_DISABLED
#include "AirLinkLink.h"
#endif

LinkConfiguration::LinkConfiguration(const QString &name, QObject *parent)
    : QObject(parent)
    , _name(name)
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;
}

LinkConfiguration::LinkConfiguration(const LinkConfiguration *copy, QObject *parent)
    : QObject(parent)
    , _link(copy->_link)
    , _name(copy->name())
    , _dynamic(copy->isDynamic())
    , _autoConnect(copy->isAutoConnect())
    , _highLatency(copy->isHighLatency())
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;

    Q_ASSERT(!_name.isEmpty());
}

LinkConfiguration::~LinkConfiguration()
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;
}

void LinkConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_ASSERT(source);

    setLink(source->_link.lock());
    setName(source->name());
    setDynamic(source->isDynamic());
    setAutoConnect(source->isAutoConnect());
    setHighLatency(source->isHighLatency());
}

LinkConfiguration *LinkConfiguration::createSettings(int type, const QString &name)
{
    LinkConfiguration *config = nullptr;

    switch (static_cast<LinkType>(type)) {
#ifndef QGC_NO_SERIAL_LINK
    case TypeSerial:
        config = new SerialConfiguration(name);
        break;
#endif
    case TypeUdp:
        config = new UDPConfiguration(name);
        break;
    case TypeTcp:
        config = new TCPConfiguration(name);
        break;
#ifdef QGC_ENABLE_BLUETOOTH
    case TypeBluetooth:
        config = new BluetoothConfiguration(name);
        break;
#endif
    case TypeLogReplay:
        config = new LogReplayConfiguration(name);
        break;
#ifdef QT_DEBUG
    case TypeMock:
        config = new MockConfiguration(name);
        break;
#endif
#ifndef QGC_AIRLINK_DISABLED
    case AirLink:
        config = new AirLinkConfiguration(name);
        break;
#endif
    case TypeLast:
    default:
        break;
    }

    return config;
}

LinkConfiguration *LinkConfiguration::duplicateSettings(const LinkConfiguration *source)
{
    LinkConfiguration *dupe = nullptr;

    switch(source->type()) {
#ifndef QGC_NO_SERIAL_LINK
    case TypeSerial:
        dupe = new SerialConfiguration(qobject_cast<const SerialConfiguration*>(source));
        break;
#endif
    case TypeUdp:
        dupe = new UDPConfiguration(qobject_cast<const UDPConfiguration*>(source));
        break;
    case TypeTcp:
        dupe = new TCPConfiguration(qobject_cast<const TCPConfiguration*>(source));
        break;
#ifdef QGC_ENABLE_BLUETOOTH
    case TypeBluetooth:
        dupe = new BluetoothConfiguration(qobject_cast<const BluetoothConfiguration*>(source));
        break;
#endif
    case TypeLogReplay:
        dupe = new LogReplayConfiguration(qobject_cast<const LogReplayConfiguration*>(source));
        break;
#ifdef QT_DEBUG
    case TypeMock:
        dupe = new MockConfiguration(qobject_cast<const MockConfiguration*>(source));
        break;
#endif
#ifndef QGC_AIRLINK_DISABLED
    case AirLink:
        dupe = new AirLinkConfiguration(qobject_cast<const AirLinkConfiguration*>(source));
        break;
#endif
    case TypeLast:
    default:
        break;
    }

    return dupe;
}

void LinkConfiguration::setName(const QString &name)
{
    if (name != _name) {
        _name = name;
        emit nameChanged(name);
    }
}

void LinkConfiguration::setLink(const SharedLinkInterfacePtr link)
{
    if (link.get() != this->link()) {
        _link = link;
        emit linkChanged();

        (void) connect(link.get(), &LinkInterface::disconnected, this, &LinkConfiguration::linkChanged, Qt::QueuedConnection);
    }
}

void LinkConfiguration::setDynamic(bool dynamic)
{
    if (dynamic != _dynamic) {
        _dynamic = dynamic;
        emit dynamicChanged();
    }
}

void LinkConfiguration::setAutoConnect(bool autoc)
{
    if (autoc != _autoConnect) {
        _autoConnect = autoc;
        emit autoConnectChanged();
    }
}

void LinkConfiguration::setHighLatency(bool hl)
{
    if (hl != _highLatency) {
        _highLatency = hl;
        emit highLatencyChanged();
    }
}
