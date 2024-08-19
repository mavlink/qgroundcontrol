/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LinkConfiguration.h"
#ifndef NO_SERIAL_LINK
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
#include "AirlinkLink.h"
#endif

LinkConfiguration::LinkConfiguration(const QString &name, QObject *parent)
    : QObject(parent)
    , _name(name)
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;
}

LinkConfiguration::LinkConfiguration(LinkConfiguration *copy, QObject *parent)
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

void LinkConfiguration::copyFrom(LinkConfiguration *source)
{
    Q_CHECK_PTR(source);

    _link = source->_link;
    _name = source->name();
    _dynamic = source->isDynamic();
    _autoConnect = source->isAutoConnect();
    _highLatency = source->isHighLatency();
}

LinkConfiguration *LinkConfiguration::createSettings(int type, const QString &name)
{
    LinkConfiguration *config = nullptr;

    switch(static_cast<LinkType>(type)) {
#ifndef NO_SERIAL_LINK
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
        config = new LogReplayLinkConfiguration(name);
        break;
#ifdef QT_DEBUG
    case TypeMock:
        config = new MockConfiguration(name);
        break;
#endif
#ifndef QGC_AIRLINK_DISABLED
    case Airlink:
        config = new AirlinkConfiguration(name);
        break;
#endif
    case TypeLast:
    default:
        break;
    }

    return config;
}

LinkConfiguration *LinkConfiguration::duplicateSettings(LinkConfiguration *source)
{
    LinkConfiguration *dupe = nullptr;

    switch(source->type()) {
#ifndef NO_SERIAL_LINK
    case TypeSerial:
        dupe = new SerialConfiguration(qobject_cast<SerialConfiguration*>(source));
        break;
#endif
    case TypeUdp:
        dupe = new UDPConfiguration(qobject_cast<UDPConfiguration*>(source));
        break;
    case TypeTcp:
        dupe = new TCPConfiguration(qobject_cast<TCPConfiguration*>(source));
        break;
#ifdef QGC_ENABLE_BLUETOOTH
    case TypeBluetooth:
        dupe = new BluetoothConfiguration(qobject_cast<BluetoothConfiguration*>(source));
        break;
#endif
    case TypeLogReplay:
        dupe = new LogReplayLinkConfiguration(qobject_cast<LogReplayLinkConfiguration*>(source));
        break;
#ifdef QT_DEBUG
    case TypeMock:
        dupe = new MockConfiguration(qobject_cast<MockConfiguration*>(source));
        break;
#endif
#ifndef QGC_AIRLINK_DISABLED
    case Airlink:
        dupe = new AirlinkConfiguration(qobject_cast<AirlinkConfiguration*>(source));
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

void LinkConfiguration::setLink(SharedLinkInterfacePtr link)
{
    if (link.get() != this->link()) {
        _link = link;
        emit linkChanged();
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
