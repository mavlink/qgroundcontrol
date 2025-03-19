/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LinkManager.h"
#include "DeviceInfo.h"
#include "LogReplayLinkController.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"
#include "AutoConnectSettings.h"
#include "TCPLink.h"
#include "UDPLink.h"

#ifdef QGC_ENABLE_BLUETOOTH
#include "BluetoothLink.h"
#endif

#ifndef QGC_NO_SERIAL_LINK
#include "SerialLink.h"
#include "GPSManager.h"
#include "PositionManager.h"
#include "UdpIODevice.h"
#include "GPSRtk.h"
#endif

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#ifndef QGC_AIRLINK_DISABLED
#include "AirLinkLink.h"
#endif

#ifdef QGC_ZEROCONF_ENABLED
#include <qmdnsengine/browser.h>
#include <qmdnsengine/cache.h>
#include <qmdnsengine/mdns.h>
#include <qmdnsengine/server.h>
#include <qmdnsengine/service.h>
#endif

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QTimer>
#include <QtQml/qqml.h>

QGC_LOGGING_CATEGORY(LinkManagerLog, "qgc.comms.linkmanager")
QGC_LOGGING_CATEGORY(LinkManagerVerboseLog, "qgc.comms.linkmanager:verbose")

Q_APPLICATION_STATIC(LinkManager, _linkManagerInstance);

LinkManager::LinkManager(QObject *parent)
    : QObject(parent)
    , _portListTimer(new QTimer(this))
    , _qmlConfigurations(new QmlObjectListModel(this))
#ifndef QGC_NO_SERIAL_LINK
    , _nmeaSocket(new UdpIODevice(this))
#endif
{
    // qCDebug(LinkManagerLog) << Q_FUNC_INFO << this;
}

LinkManager::~LinkManager()
{
    // qCDebug(LinkManagerLog) << Q_FUNC_INFO << this;
}

LinkManager *LinkManager::instance()
{
    return _linkManagerInstance();
}

void LinkManager::registerQmlTypes()
{
    (void) qmlRegisterUncreatableType<LinkManager>("QGroundControl",       1, 0, "LinkManager",         "Reference only");
    (void) qmlRegisterUncreatableType<LinkConfiguration>("QGroundControl", 1, 0, "LinkConfiguration",   "Reference only");
    (void) qmlRegisterUncreatableType<LinkInterface>("QGroundControl",     1, 0, "LinkInterface",       "Reference only");

    (void) qmlRegisterUncreatableType<LinkInterface>("QGroundControl.Vehicle", 1, 0, "LinkInterface", "Reference only");
    (void) qmlRegisterUncreatableType<LogReplayLink>("QGroundControl",         1, 0, "LogReplayLink", "Reference only");
    (void) qmlRegisterType<LogReplayLinkController> ("QGroundControl",         1, 0, "LogReplayLinkController");

    (void) qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    (void) qRegisterMetaType<LinkInterface*>("LinkInterface*");
#ifndef QGC_NO_SERIAL_LINK
    (void) qRegisterMetaType<QGCSerialPortInfo>("QGCSerialPortInfo");
#endif
}

void LinkManager::init()
{
    _autoConnectSettings = SettingsManager::instance()->autoConnectSettings();

    if (!qgcApp()->runningUnitTests()) {
        (void) connect(_portListTimer, &QTimer::timeout, this, &LinkManager::_updateAutoConnectLinks);
        _portListTimer->start(_autoconnectUpdateTimerMSecs); // timeout must be long enough to get past bootloader on second pass
    }
}

QmlObjectListModel *LinkManager::_qmlLinkConfigurations()
{
    return _qmlConfigurations;
}

void LinkManager::createConnectedLink(const LinkConfiguration *config)
{
    for (SharedLinkConfigurationPtr &sharedConfig : _rgLinkConfigs) {
        if (sharedConfig.get() == config) {
            createConnectedLink(sharedConfig);
        }
    }
}

bool LinkManager::createConnectedLink(SharedLinkConfigurationPtr &config)
{
    SharedLinkInterfacePtr link = nullptr;

    switch(config->type()) {
#ifndef QGC_NO_SERIAL_LINK
    case LinkConfiguration::TypeSerial:
        link = std::make_shared<SerialLink>(config);
        break;
#endif
    case LinkConfiguration::TypeUdp:
        link = std::make_shared<UDPLink>(config);
        break;
    case LinkConfiguration::TypeTcp:
        link = std::make_shared<TCPLink>(config);
        break;
#ifdef QGC_ENABLE_BLUETOOTH
    case LinkConfiguration::TypeBluetooth:
        link = std::make_shared<BluetoothLink>(config);
        break;
#endif
    case LinkConfiguration::TypeLogReplay:
        link = std::make_shared<LogReplayLink>(config);
        break;
#ifdef QT_DEBUG
    case LinkConfiguration::TypeMock:
        link = std::make_shared<MockLink>(config);
        break;
#endif
#ifndef QGC_AIRLINK_DISABLED
    case LinkConfiguration::AirLink:
        link = std::make_shared<AirLinkLink>(config);
        break;
#endif
    case LinkConfiguration::TypeLast:
    default:
        break;
    }

    if (!link) {
        return false;
    }

    if (!link->_allocateMavlinkChannel()) {
        qCWarning(LinkManagerLog) << "Link failed to setup mavlink channels";
        return false;
    }

    _rgLinks.append(link);
    config->setLink(link);

    (void) connect(link.get(), &LinkInterface::communicationError, qgcApp(), &QGCApplication::showAppMessage);
    (void) connect(link.get(), &LinkInterface::bytesReceived, MAVLinkProtocol::instance(), &MAVLinkProtocol::receiveBytes);
    (void) connect(link.get(), &LinkInterface::bytesSent, MAVLinkProtocol::instance(), &MAVLinkProtocol::logSentBytes);
    (void) connect(link.get(), &LinkInterface::disconnected, this, &LinkManager::_linkDisconnected);

    MAVLinkProtocol::instance()->resetMetadataForLink(link.get());
    MAVLinkProtocol::instance()->setVersion(MAVLinkProtocol::instance()->getCurrentVersion());

    if (!link->_connect()) {
        link->_freeMavlinkChannel();
        _rgLinks.removeAt(_rgLinks.indexOf(link));
        config->setLink(nullptr);
        return false;
    }

    return true;
}

SharedLinkInterfacePtr LinkManager::mavlinkForwardingLink()
{
    for (SharedLinkInterfacePtr &link : _rgLinks) {
        const SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
        if ((linkConfig->type() == LinkConfiguration::TypeUdp) && (linkConfig->name() == _mavlinkForwardingLinkName)) {
            return link;
        }
    }

    return nullptr;
}

SharedLinkInterfacePtr LinkManager::mavlinkForwardingSupportLink()
{
    for (SharedLinkInterfacePtr &link : _rgLinks) {
        const SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
        if ((linkConfig->type() == LinkConfiguration::TypeUdp) && (linkConfig->name() == _mavlinkForwardingSupportLinkName)) {
            return link;
        }
    }

    return nullptr;
}

void LinkManager::disconnectAll()
{
    const QList<SharedLinkInterfacePtr> links = _rgLinks;
    for (const SharedLinkInterfacePtr &sharedLink: links) {
        sharedLink->disconnect();
    }
}

void LinkManager::_linkDisconnected()
{
    LinkInterface* const link = qobject_cast<LinkInterface*>(sender());

    if (!link || !containsLink(link)) {
        return;
    }

    (void) disconnect(link, &LinkInterface::communicationError, qgcApp(), &QGCApplication::showAppMessage);
    (void) disconnect(link, &LinkInterface::bytesReceived, MAVLinkProtocol::instance(), &MAVLinkProtocol::receiveBytes);
    (void) disconnect(link, &LinkInterface::bytesSent, MAVLinkProtocol::instance(), &MAVLinkProtocol::logSentBytes);
    (void) disconnect(link, &LinkInterface::disconnected, this, &LinkManager::_linkDisconnected);

    link->_freeMavlinkChannel();

    for (auto it = _rgLinks.begin(); it != _rgLinks.end(); ++it) {
        if (it->get() == link) {
            qCDebug(LinkManagerLog) << Q_FUNC_INFO << it->get()->linkConfiguration()->name() << it->use_count();
            (void) _rgLinks.erase(it);
            return;
        }
    }
}

SharedLinkInterfacePtr LinkManager::sharedLinkInterfacePointerForLink(const LinkInterface *link)
{
    for (SharedLinkInterfacePtr &sharedLink: _rgLinks) {
        if (sharedLink.get() == link) {
            return sharedLink;
        }
    }

    qCWarning(LinkManagerLog) << "returning nullptr";
    return SharedLinkInterfacePtr(nullptr);
}

bool LinkManager::_connectionsSuspendedMsg() const
{
    if (_connectionsSuspended) {
        qgcApp()->showAppMessage(tr("Connect not allowed: %1").arg(_connectionsSuspendedReason));
        return true;
    }

    return false;
}

void LinkManager::saveLinkConfigurationList()
{
    QSettings settings;
    settings.remove(LinkConfiguration::settingsRoot());

    int trueCount = 0;
    for (int i = 0; i < _rgLinkConfigs.count(); i++) {
        SharedLinkConfigurationPtr linkConfig = _rgLinkConfigs[i];
        if (!linkConfig) {
            qCWarning(LinkManagerLog) << "Internal error for link configuration in LinkManager";
            continue;
        }

        if (linkConfig->isDynamic()) {
            continue;
        }

        const QString root = LinkConfiguration::settingsRoot() + QStringLiteral("/Link%1").arg(trueCount++);
        settings.setValue(root + "/name", linkConfig->name());
        settings.setValue(root + "/type", linkConfig->type());
        settings.setValue(root + "/auto", linkConfig->isAutoConnect());
        settings.setValue(root + "/high_latency", linkConfig->isHighLatency());
        linkConfig->saveSettings(settings, root);
    }

    const QString root = QString(LinkConfiguration::settingsRoot());
    settings.setValue(root + "/count", trueCount);
}

void LinkManager::loadLinkConfigurationList()
{
    QSettings settings;
    // Is the group even there?
    if (settings.contains(LinkConfiguration::settingsRoot() + "/count")) {
        // Find out how many configurations we have
        const int count = settings.value(LinkConfiguration::settingsRoot() + "/count").toInt();
        for (int i = 0; i < count; i++) {
            const QString root = LinkConfiguration::settingsRoot() + QStringLiteral("/Link%1").arg(i);
            if (!settings.contains(root + "/type")) {
                qCWarning(LinkManagerLog) << "Link Configuration" << root << "has no type.";
                continue;
            }

            LinkConfiguration::LinkType type = static_cast<LinkConfiguration::LinkType>(settings.value(root + "/type").toInt());
            if (type >= LinkConfiguration::TypeLast) {
                qCWarning(LinkManagerLog) << "Link Configuration" << root << "an invalid type:" << type;
                continue;
            }

            if (!settings.contains(root + "/name")) {
                qCWarning(LinkManagerLog) << "Link Configuration" << root << "has no name.";
                continue;
            }

            const QString name = settings.value(root + "/name").toString();
            if (name.isEmpty()) {
                qCWarning(LinkManagerLog) << "Link Configuration" << root << "has an empty name.";
                continue;
            }

            LinkConfiguration* link = nullptr;
            switch(type) {
#ifndef QGC_NO_SERIAL_LINK
            case LinkConfiguration::TypeSerial:
                link = new SerialConfiguration(name);
                break;
#endif
            case LinkConfiguration::TypeUdp:
                link = new UDPConfiguration(name);
                break;
            case LinkConfiguration::TypeTcp:
                link = new TCPConfiguration(name);
                break;
#ifdef QGC_ENABLE_BLUETOOTH
            case LinkConfiguration::TypeBluetooth:
                link = new BluetoothConfiguration(name);
                break;
#endif
            case LinkConfiguration::TypeLogReplay:
                link = new LogReplayConfiguration(name);
                break;
#ifdef QT_DEBUG
            case LinkConfiguration::TypeMock:
                link = new MockConfiguration(name);
                break;
#endif
#ifndef QGC_AIRLINK_DISABLED
            case LinkConfiguration::AirLink:
                link = new AirLinkConfiguration(name);
                break;
#endif
            case LinkConfiguration::TypeLast:
            default:
                break;
            }

            if (link) {
                const bool autoConnect = settings.value(root + "/auto").toBool();
                link->setAutoConnect(autoConnect);
                const bool highLatency = settings.value(root + "/high_latency").toBool();
                link->setHighLatency(highLatency);
                link->loadSettings(settings, root);
                addConfiguration(link);
            }
        }
    }

    // Enable automatic Serial PX4/3DR Radio hunting
    _configurationsLoaded = true;
}

void LinkManager::_addUDPAutoConnectLink()
{
    if (!_autoConnectSettings->autoConnectUDP()->rawValue().toBool()) {
        return;
    }

    for (SharedLinkInterfacePtr &link : _rgLinks) {
        const SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
        if ((linkConfig->type() == LinkConfiguration::TypeUdp) && (linkConfig->name() == _defaultUDPLinkName)) {
            return;
        }
    }

    qCDebug(LinkManagerLog) << "New auto-connect UDP port added";
    UDPConfiguration* const udpConfig = new UDPConfiguration(_defaultUDPLinkName);
    udpConfig->setDynamic(true);
    udpConfig->setAutoConnect(true);
    SharedLinkConfigurationPtr config = addConfiguration(udpConfig);
    createConnectedLink(config);
}

void LinkManager::_addMAVLinkForwardingLink()
{
    if (!SettingsManager::instance()->mavlinkSettings()->forwardMavlink()->rawValue().toBool()) {
        return;
    }

    for (const SharedLinkInterfacePtr &link : _rgLinks) {
        const SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
        if ((linkConfig->type() == LinkConfiguration::TypeUdp) && (linkConfig->name() == _mavlinkForwardingLinkName)) {
            // TODO: should we check if the host/port matches the mavlinkForwardHostName setting and update if it does not match?
            return;
        }
    }

    const QString hostName = SettingsManager::instance()->mavlinkSettings()->forwardMavlinkHostName()->rawValue().toString();
    _createDynamicForwardLink(_mavlinkForwardingLinkName, hostName);
}

#ifdef QGC_ZEROCONF_ENABLED
void LinkManager::_addZeroConfAutoConnectLink()
{
    if (!_autoConnectSettings->autoConnectZeroConf()->rawValue().toBool()) {
        return;
    }

    static QSharedPointer<QMdnsEngine::Server> server;
    static QSharedPointer<QMdnsEngine::Browser> browser;
    server.reset(new QMdnsEngine::Server());
    browser.reset(new QMdnsEngine::Browser(server.get(), QMdnsEngine::MdnsBrowseType));

    const auto checkIfConnectionLinkExist = [this](LinkConfiguration::LinkType linkType, const QString &linkName) {
        for (const SharedLinkInterfacePtr &link : std::as_const(_rgLinks)) {
            const SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
            if ((linkConfig->type() == linkType) && (linkConfig->name() == linkName)) {
                return true;
            }
        }

        return false;
    };

    (void) connect(browser.get(), &QMdnsEngine::Browser::serviceAdded, this, [checkIfConnectionLinkExist, this](const QMdnsEngine::Service &service) {
        qCDebug(LinkManagerLog) << "Found Zero-Conf:" << service.type() << service.name() << service.hostname() << service.port() << service.attributes();

        if (!service.type().startsWith("_mavlink")) {
            qCWarning(LinkManagerLog) << "Invalid ZeroConf SericeType" << service.type();
            return;
        }

        // Windows doesnt accept trailling dots in mdns
        // http://www.dns-sd.org/trailingdotsindomainnames.html
        QString hostname = service.hostname();
        if (hostname.endsWith('.')) {
            hostname.chop(1);
        }

        if (service.type().startsWith("_mavlink._udp")) {
            static const QString udpName = QStringLiteral("ZeroConf UDP");
            if (checkIfConnectionLinkExist(LinkConfiguration::TypeUdp, udpName)) {
                qCDebug(LinkManagerLog) << "Connection already exist";
                return;
            }

            UDPConfiguration *const link = new UDPConfiguration(udpName);
            link->addHost(hostname, service.port());
            link->setAutoConnect(true);
            link->setDynamic(true);
            SharedLinkConfigurationPtr config = addConfiguration(link);
            if (!createConnectedLink(config)) {
                qCWarning(LinkManagerLog) << "Failed to create" << udpName;
            }
        } else if (service.type().startsWith("_mavlink._tcp")) {
            static QString tcpName = QStringLiteral("ZeroConf TCP");
            if (checkIfConnectionLinkExist(LinkConfiguration::TypeTcp, tcpName)) {
                qCDebug(LinkManagerLog) << "Connection already exist";
                return;
            }

            TCPConfiguration *const link = new TCPConfiguration(tcpName);
            link->setHost(hostname);
            link->setPort(service.port());
            link->setAutoConnect(true);
            link->setDynamic(true);
            SharedLinkConfigurationPtr config = addConfiguration(link);
            if (!createConnectedLink(config)) {
                qCWarning(LinkManagerLog) << "Failed to create" << tcpName;
            }
        }
    });
}
#endif

void LinkManager::_updateAutoConnectLinks()
{
    if (_connectionsSuspended) {
        return;
    }

    _addUDPAutoConnectLink();
    _addMAVLinkForwardingLink();
#ifdef QGC_ZEROCONF_ENABLED
    _addZeroConfAutoConnectLink();
#endif

    // check to see if nmea gps is configured for UDP input, if so, set it up to connect
    if (_autoConnectSettings->autoConnectNmeaPort()->cookedValueString() == "UDP Port") {
        if ((_nmeaSocket->localPort() != _autoConnectSettings->nmeaUdpPort()->rawValue().toUInt()) || (_nmeaSocket->state() != UdpIODevice::BoundState)) {
            qCDebug(LinkManagerLog) << "Changing port for UDP NMEA stream";
            _nmeaSocket->close();
            _nmeaSocket->bind(QHostAddress::AnyIPv4, _autoConnectSettings->nmeaUdpPort()->rawValue().toUInt());
            QGCPositionManager::instance()->setNmeaSourceDevice(_nmeaSocket);
        }
#ifndef QGC_NO_SERIAL_LINK
        if (_nmeaPort) {
            _nmeaPort->close();
            delete _nmeaPort;
            _nmeaPort = nullptr;
            _nmeaDeviceName = "";
        }
#endif
    } else {
        _nmeaSocket->close();
    }

#ifndef QGC_NO_SERIAL_LINK
    _addSerialAutoConnectLink();
#endif
}

void LinkManager::shutdown()
{
    setConnectionsSuspended(tr("Shutdown"));
    disconnectAll();

    // Wait for all the vehicles to go away to ensure an orderly shutdown and deletion of all objects
    while (MultiVehicleManager::instance()->vehicles()->count()) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

QStringList LinkManager::linkTypeStrings() const
{
    //-- Must follow same order as enum LinkType in LinkConfiguration.h
    static QStringList list;
    if (!list.isEmpty()) {
        return list;
    }

#ifndef QGC_NO_SERIAL_LINK
    list += tr("Serial");
#endif
    list += tr("UDP");
    list += tr("TCP");
#ifdef QGC_ENABLE_BLUETOOTH
    list += tr("Bluetooth");
#endif
#ifdef QT_DEBUG
    list += tr("Mock Link");
#endif
#ifndef QGC_AIRLINK_DISABLED
    list += tr("AirLink");
#endif
    list += tr("Log Replay");

    if (list.size() != static_cast<int>(LinkConfiguration::TypeLast)) {
        qCWarning(LinkManagerLog) << "Internal error";
    }

    return list;
}

void LinkManager::endConfigurationEditing(LinkConfiguration *config, LinkConfiguration *editedConfig)
{
    if (!config || !editedConfig) {
        qCWarning(LinkManagerLog) << "Internal error";
        return;
    }

    config->copyFrom(editedConfig);
    saveLinkConfigurationList();
    emit config->nameChanged(config->name());
    // Discard temporary duplicate
    delete editedConfig;
}

void LinkManager::endCreateConfiguration(LinkConfiguration *config)
{
    if (!config) {
        qCWarning(LinkManagerLog) << "Internal error";
        return;
    }

    addConfiguration(config);
    saveLinkConfigurationList();
}

LinkConfiguration *LinkManager::createConfiguration(int type, const QString &name)
{
#ifndef QGC_NO_SERIAL_LINK
    if (static_cast<LinkConfiguration::LinkType>(type) == LinkConfiguration::TypeSerial) {
        _updateSerialPorts();
    }
#endif

    return LinkConfiguration::createSettings(type, name);
}

LinkConfiguration *LinkManager::startConfigurationEditing(LinkConfiguration *config)
{
    if (!config) {
        qCWarning(LinkManagerLog) << "Internal error";
        return nullptr;
    }

#ifndef QGC_NO_SERIAL_LINK
    if (config->type() == LinkConfiguration::TypeSerial) {
        _updateSerialPorts();
    }
#endif

    return LinkConfiguration::duplicateSettings(config);
}

void LinkManager::removeConfiguration(LinkConfiguration *config)
{
    if (!config) {
        qCWarning(LinkManagerLog) << "Internal error";
        return;
    }

    LinkInterface* const link = config->link();
    if (link) {
        link->disconnect();
    }

    _removeConfiguration(config);
    saveLinkConfigurationList();
}

void LinkManager::createMavlinkForwardingSupportLink()
{
    const QString hostName = SettingsManager::instance()->mavlinkSettings()->forwardMavlinkAPMSupportHostName()->rawValue().toString();
    _createDynamicForwardLink(_mavlinkForwardingSupportLinkName, hostName);
    _mavlinkSupportForwardingEnabled = true;
    emit mavlinkSupportForwardingEnabledChanged();
}

void LinkManager::_removeConfiguration(const LinkConfiguration *config)
{
    (void) _qmlConfigurations->removeOne(config);

    for (auto it = _rgLinkConfigs.begin(); it != _rgLinkConfigs.end(); ++it) {
        if (it->get() == config) {
            (void) _rgLinkConfigs.erase(it);
            return;
        }
    }

    qCWarning(LinkManagerLog) << Q_FUNC_INFO << "called with unknown config";
}

bool LinkManager::isBluetoothAvailable()
{
    return QGCDeviceInfo::isBluetoothAvailable();
}

bool LinkManager::containsLink(const LinkInterface *link) const
{
    for (const SharedLinkInterfacePtr &sharedLink : _rgLinks) {
        if (sharedLink.get() == link) {
            return true;
        }
    }

    return false;
}

SharedLinkConfigurationPtr LinkManager::addConfiguration(LinkConfiguration *config)
{
    (void) _qmlConfigurations->append(config);
    (void) _rgLinkConfigs.append(SharedLinkConfigurationPtr(config));

    return _rgLinkConfigs.last();
}

void LinkManager::startAutoConnectedLinks()
{
    for (SharedLinkConfigurationPtr &sharedConfig : _rgLinkConfigs) {
        if (sharedConfig->isAutoConnect()) {
            createConnectedLink(sharedConfig);
        }
    }
}

uint8_t LinkManager::allocateMavlinkChannel()
{
    for (uint8_t mavlinkChannel = 0; mavlinkChannel < MAVLINK_COMM_NUM_BUFFERS; mavlinkChannel++) {
        if (_mavlinkChannelsUsedBitMask & (1 << mavlinkChannel)) {
            continue;
        }

        mavlink_reset_channel_status(mavlinkChannel);
        mavlink_status_t* const mavlinkStatus = mavlink_get_channel_status(mavlinkChannel);
        mavlinkStatus->flags |= MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
        _mavlinkChannelsUsedBitMask |= (1 << mavlinkChannel);
        qCDebug(LinkManagerLog) << "allocateMavlinkChannel" << mavlinkChannel;
        return mavlinkChannel;
    }

    qWarning(LinkManagerLog) << "allocateMavlinkChannel: all channels reserved!";
    return invalidMavlinkChannel();
}

void LinkManager::freeMavlinkChannel(uint8_t channel)
{
    qCDebug(LinkManagerLog) << "freeMavlinkChannel" << channel;

    if (invalidMavlinkChannel() == channel) {
        return;
    }

    _mavlinkChannelsUsedBitMask &= ~(1 << channel);
}

LogReplayLink *LinkManager::startLogReplay(const QString &logFile)
{
    LogReplayConfiguration* const linkConfig = new LogReplayConfiguration(tr("Log Replay"));
    linkConfig->setLogFilename(logFile);
    linkConfig->setName(linkConfig->logFilenameShort());

    SharedLinkConfigurationPtr sharedConfig = addConfiguration(linkConfig);
    if (createConnectedLink(sharedConfig)) {
        return qobject_cast<LogReplayLink*>(sharedConfig->link());
    }

    return nullptr;
}

void LinkManager::_createDynamicForwardLink(const char *linkName, const QString &hostName)
{
    UDPConfiguration* const udpConfig = new UDPConfiguration(linkName);

    udpConfig->setDynamic(true);
    udpConfig->setForwarding(true);
    udpConfig->addHost(hostName);

    SharedLinkConfigurationPtr config = addConfiguration(udpConfig);
    createConnectedLink(config);

    qCDebug(LinkManagerLog) << "New dynamic MAVLink forwarding port added:" << linkName << " hostname:" << hostName;
}

bool LinkManager::isLinkUSBDirect(const LinkInterface *link)
{
#ifndef QGC_NO_SERIAL_LINK
    const SerialLink* const serialLink = qobject_cast<const SerialLink*>(link);
    if (!serialLink) {
        return false;
    }

    const SharedLinkConfigurationPtr config = serialLink->linkConfiguration();
    if (!config) {
        return false;
    }

    const SerialConfiguration* const serialConfig = qobject_cast<const SerialConfiguration*>(config.get());
    if (serialConfig && serialConfig->usbDirect()) {
        return link;
    }
#endif

    return false;
}

void LinkManager::resetMavlinkSigning()
{
    for (const SharedLinkInterfacePtr &sharedLink: _rgLinks) {
        sharedLink->initMavlinkSigning();
    }
}

#ifndef QGC_NO_SERIAL_LINK // Serial Only Functions

void LinkManager::_addSerialAutoConnectLink()
{
    QList<QGCSerialPortInfo> portList;
#ifdef Q_OS_ANDROID
    // Android builds only support a single serial connection. Repeatedly calling availablePorts after that one serial
    // port is connected leaks file handles due to a bug somewhere in android serial code. In order to work around that
    // bug after we connect the first serial port we stop probing for additional ports.
    if (!_isSerialPortConnected()) {
        portList = QGCSerialPortInfo::availablePorts();
    }
#else
    portList = QGCSerialPortInfo::availablePorts();
#endif

    QStringList currentPorts;
    for (const QGCSerialPortInfo &portInfo: portList) {
        qCDebug(LinkManagerVerboseLog) << "-----------------------------------------------------";
        qCDebug(LinkManagerVerboseLog) << "portName:          " << portInfo.portName();
        qCDebug(LinkManagerVerboseLog) << "systemLocation:    " << portInfo.systemLocation();
        qCDebug(LinkManagerVerboseLog) << "description:       " << portInfo.description();
        qCDebug(LinkManagerVerboseLog) << "manufacturer:      " << portInfo.manufacturer();
        qCDebug(LinkManagerVerboseLog) << "serialNumber:      " << portInfo.serialNumber();
        qCDebug(LinkManagerVerboseLog) << "vendorIdentifier:  " << portInfo.vendorIdentifier();
        qCDebug(LinkManagerVerboseLog) << "productIdentifier: " << portInfo.productIdentifier();

        currentPorts << portInfo.systemLocation();

        QGCSerialPortInfo::BoardType_t boardType;
        QString boardName;

        // check to see if nmea gps is configured for current Serial port, if so, set it up to connect
        if (portInfo.systemLocation().trimmed() == _autoConnectSettings->autoConnectNmeaPort()->cookedValueString()) {
            if (portInfo.systemLocation().trimmed() != _nmeaDeviceName) {
                _nmeaDeviceName = portInfo.systemLocation().trimmed();
                qCDebug(LinkManagerLog) << "Configuring nmea port" << _nmeaDeviceName;
                QSerialPort* newPort = new QSerialPort(portInfo, this);
                _nmeaBaud = _autoConnectSettings->autoConnectNmeaBaud()->cookedValue().toUInt();
                newPort->setBaudRate(static_cast<qint32>(_nmeaBaud));
                qCDebug(LinkManagerLog) << "Configuring nmea baudrate" << _nmeaBaud;
                // This will stop polling old device if previously set
                QGCPositionManager::instance()->setNmeaSourceDevice(newPort);
                if (_nmeaPort) {
                    delete _nmeaPort;
                }
                _nmeaPort = newPort;
            } else if (_autoConnectSettings->autoConnectNmeaBaud()->cookedValue().toUInt() != _nmeaBaud) {
                _nmeaBaud = _autoConnectSettings->autoConnectNmeaBaud()->cookedValue().toUInt();
                _nmeaPort->setBaudRate(static_cast<qint32>(_nmeaBaud));
                qCDebug(LinkManagerLog) << "Configuring nmea baudrate" << _nmeaBaud;
            }
        } else if (portInfo.getBoardInfo(boardType, boardName)) {
            // Should we be auto-connecting to this board type?
            if (!_allowAutoConnectToBoard(boardType)) {
                continue;
            }

            if (portInfo.isBootloader()) {
                // Don't connect to bootloader
                qCDebug(LinkManagerLog) << "Waiting for bootloader to finish" << portInfo.systemLocation();
                continue;
            }
            if (_portAlreadyConnected(portInfo.systemLocation()) || (_autoConnectRTKPort == portInfo.systemLocation())) {
                qCDebug(LinkManagerVerboseLog) << "Skipping existing autoconnect" << portInfo.systemLocation();
            } else if (!_autoconnectPortWaitList.contains(portInfo.systemLocation())) {
                // We don't connect to the port the first time we see it. The ability to correctly detect whether we
                // are in the bootloader is flaky from a cross-platform standpoint. So by putting it on a wait list
                // and only connect on the second pass we leave enough time for the board to boot up.
                qCDebug(LinkManagerLog) << "Waiting for next autoconnect pass" << portInfo.systemLocation() << boardName;
                _autoconnectPortWaitList[portInfo.systemLocation()] = 1;
            } else if ((++_autoconnectPortWaitList[portInfo.systemLocation()] * _autoconnectUpdateTimerMSecs) > _autoconnectConnectDelayMSecs) {
                SerialConfiguration* pSerialConfig = nullptr;
                _autoconnectPortWaitList.remove(portInfo.systemLocation());
                switch (boardType) {
                case QGCSerialPortInfo::BoardTypePixhawk:
                    pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName, portInfo.portName().trimmed()));
                    pSerialConfig->setUsbDirect(true);
                    break;
                case QGCSerialPortInfo::BoardTypeSiKRadio:
                    pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName, portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardTypeOpenPilot:
                    pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName, portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardTypeRTKGPS:
                    qCDebug(LinkManagerLog) << "RTK GPS auto-connected" << portInfo.portName().trimmed();
                    _autoConnectRTKPort = portInfo.systemLocation();
                    GPSManager::instance()->gpsRtk()->connectGPS(portInfo.systemLocation(), boardName);
                    break;
                default:
                    qCWarning(LinkManagerLog) << "Internal error: Unknown board type" << boardType;
                    continue;
                }

                if (pSerialConfig) {
                    qCDebug(LinkManagerLog) << "New auto-connect port added: " << pSerialConfig->name() << portInfo.systemLocation();
                    pSerialConfig->setBaud((boardType == QGCSerialPortInfo::BoardTypeSiKRadio) ? 57600 : 115200);
                    pSerialConfig->setDynamic(true);
                    pSerialConfig->setPortName(portInfo.systemLocation());
                    pSerialConfig->setAutoConnect(true);

                    SharedLinkConfigurationPtr sharedConfig(pSerialConfig);
                    createConnectedLink(sharedConfig);
                }
            }
        }
    }

    // Check for RTK GPS connection gone
    if (!_autoConnectRTKPort.isEmpty() && !currentPorts.contains(_autoConnectRTKPort)) {
        qCDebug(LinkManagerLog) << "RTK GPS disconnected" << _autoConnectRTKPort;
        GPSManager::instance()->gpsRtk()->disconnectGPS();
        _autoConnectRTKPort.clear();
    }
}

bool LinkManager::_allowAutoConnectToBoard(QGCSerialPortInfo::BoardType_t boardType) const
{
    switch (boardType) {
    case QGCSerialPortInfo::BoardTypePixhawk:
        if (_autoConnectSettings->autoConnectPixhawk()->rawValue().toBool()) {
            return true;
        }
        break;
    case QGCSerialPortInfo::BoardTypeSiKRadio:
        if (_autoConnectSettings->autoConnectSiKRadio()->rawValue().toBool()) {
            return true;
        }
        break;
    case QGCSerialPortInfo::BoardTypeOpenPilot:
        if (_autoConnectSettings->autoConnectLibrePilot()->rawValue().toBool()) {
            return true;
        }
        break;
    case QGCSerialPortInfo::BoardTypeRTKGPS:
        if (_autoConnectSettings->autoConnectRTKGPS()->rawValue().toBool() && !GPSManager::instance()->gpsRtk()->connected()) {
            return true;
        }
        break;
    default:
        qCWarning(LinkManagerLog) << "Internal error: Unknown board type" << boardType;
        return false;
    }

    return false;
}

bool LinkManager::_portAlreadyConnected(const QString &portName) const
{
    const QString searchPort = portName.trimmed();
    for (const SharedLinkInterfacePtr &linkInterface : _rgLinks) {
        const SharedLinkConfigurationPtr linkConfig = linkInterface->linkConfiguration();
        const SerialConfiguration* const serialConfig = qobject_cast<const SerialConfiguration*>(linkConfig.get());
        if (serialConfig && (serialConfig->portName() == searchPort)) {
            return true;
        }
    }

    return false;
}

void LinkManager::_updateSerialPorts()
{
    _commPortList.clear();
    _commPortDisplayList.clear();
    const QList<QGCSerialPortInfo> portList = QGCSerialPortInfo::availablePorts();
    for (const QGCSerialPortInfo &info: portList) {
        const QString port = info.systemLocation().trimmed();
        _commPortList += port;
        _commPortDisplayList += SerialConfiguration::cleanPortDisplayName(port);
    }
}

QStringList LinkManager::serialPortStrings()
{
    if (_commPortDisplayList.isEmpty()) {
        _updateSerialPorts();
    }

    return _commPortDisplayList;
}

QStringList LinkManager::serialPorts()
{
    if (_commPortList.isEmpty()) {
        _updateSerialPorts();
    }

    return _commPortList;
}

QStringList LinkManager::serialBaudRates()
{
    return SerialConfiguration::supportedBaudRates();
}

bool LinkManager::_isSerialPortConnected() const
{
    for (const SharedLinkInterfacePtr &link: _rgLinks) {
        if (qobject_cast<const SerialLink*>(link.get())) {
            return true;
        }
    }

    return false;
}

#endif // QGC_NO_SERIAL_LINK
