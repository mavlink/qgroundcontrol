#include "SerialAutoConnect.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSet>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"
#include "SerialLink.h"

QGC_LOGGING_CATEGORY(SerialAutoConnectLog, "Comms.Serial.SerialAutoConnect")

SerialAutoConnect::SerialAutoConnect(SerialPortManager& ports, Connect connect)
    : _ports(ports), _connect(std::move(connect))
{
    qCDebug(SerialAutoConnectLog) << this;
}

SerialAutoConnect::~SerialAutoConnect()
{
    qCDebug(SerialAutoConnectLog) << this;
}

void SerialAutoConnect::update(const QList<SerialPortManager::Port>& ports, const Options& options)
{
    const auto isAbsent = [&ports](const auto& entry) {
        return std::none_of(ports.cbegin(), ports.cend(),
                            [&entry](const auto& port) { return port.systemLocation == entry.key(); });
    };
    _identities.removeIf(isAbsent);
    for (const auto& port : ports) {
        const Identity identity{port.physicalDeviceId, port.boardType, port.bootloader};
        const auto previous = _identities.constFind(port.systemLocation);
        if (previous != _identities.cend() && previous.value() != identity) {
            _configs.remove(port.systemLocation);
            _waitingPorts.remove(port.systemLocation);
        }
        _identities.insert(port.systemLocation, identity);
    }
    _configs.removeIf(isAbsent);
    _waitingPorts.removeIf(isAbsent);

    QSet<QString> seenDevices;
    for (const SerialPortManager::Port& port : ports) {
        if (!_allowed(port.boardType, options) || port.bootloader ||
            _ports.isAutoConnectExcluded(port.systemLocation)) {
            continue;
        }
        // Select one MAVLink interface per physical device, even while its port is reserved or retrying.
        if (!port.physicalDeviceId.isEmpty()) {
            if (seenDevices.contains(port.physicalDeviceId)) {
                continue;
            }
            seenDevices.insert(port.physicalDeviceId);
        }
        if (!_ports.canAutoConnectPort(port.systemLocation)) {
            continue;
        }
        auto config = _configs.value(port.systemLocation);
        if (config) {
            if (!config->link() && !config->suppressAutoReconnect() && config->reconnectReady()) {
                config->noteReconnectAttempt();
                _connect(config);
            }
            continue;
        }
        if (!_waitingPorts.contains(port.systemLocation)) {
            _waitingPorts.insert(port.systemLocation, QDeadlineTimer(kConnectDelayMs));
            continue;
        }
        if (!_waitingPorts.value(port.systemLocation).hasExpired()) {
            continue;
        }
        _waitingPorts.remove(port.systemLocation);
        auto* serialConfig = new SerialConfiguration(
            QCoreApplication::translate("LinkManager", "%1 on %2 (AutoConnect)").arg(port.boardName, port.portName));
        serialConfig->setUsbDirect(port.boardType == QGCSerialPortInfo::BoardTypePixhawk);
        serialConfig->setBaud(port.boardType == QGCSerialPortInfo::BoardTypeSiKRadio ? 57600 : 115200);
        serialConfig->setDynamic(true);
        serialConfig->setPortName(port.systemLocation);
        serialConfig->setAutoConnect(true);
        config.reset(serialConfig);
        _configs.insert(port.systemLocation, config);
        config->noteReconnectAttempt();
        _connect(config);
    }
}

bool SerialAutoConnect::_allowed(QGCSerialPortInfo::BoardType_t boardType, const Options& options)
{
    switch (boardType) {
        case QGCSerialPortInfo::BoardTypePixhawk:
            return options.pixhawk;
        case QGCSerialPortInfo::BoardTypeSiKRadio:
            return options.sikRadio;
        case QGCSerialPortInfo::BoardTypeOpenPilot:
            return options.openPilot;
        default:
            return false;
    }
}
