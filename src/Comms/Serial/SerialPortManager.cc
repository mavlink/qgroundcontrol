#include "SerialPortManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QSet>

#include <algorithm>
#include <iterator>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SerialPortManagerLog, "Comms.Serial.SerialPortManager")
Q_APPLICATION_STATIC(SerialPortManager, _serialPortManager)

SerialPortManager::SerialPortManager(QObject* parent, Enumerator enumerator)
    : QObject(parent), _enumerator(enumerator ? std::move(enumerator) : &_enumeratePorts)
{}

SerialPortManager* SerialPortManager::instance()
{
    return _serialPortManager();
}

QList<SerialPortManager::Port> SerialPortManager::_enumeratePorts()
{
    QList<Port> ports;
    for (const QGCSerialPortInfo& info : QGCSerialPortInfo::availablePorts()) {
        Port port;
        if (info.hasVendorIdentifier() && info.hasProductIdentifier() && !info.serialNumber().isEmpty() &&
            info.serialNumber() != QStringLiteral("0")) {
            port.physicalDeviceId = QStringLiteral("%1:%2:%3")
                                        .arg(info.vendorIdentifier())
                                        .arg(info.productIdentifier())
                                        .arg(info.serialNumber());
        }
        port.description = info.description();
        port.systemLocation = info.systemLocation().trimmed();
        port.portName = info.portName().trimmed();
        port.displayName = port.portName;
#ifdef Q_OS_ANDROID
        const QString label = port.description.isEmpty() ? info.manufacturer() : port.description;
        if (!label.isEmpty()) {
            port.displayName = QStringLiteral("%1 (%2)").arg(label, port.portName);
        }
#endif
        (void) info.getBoardInfo(port.boardType, port.boardName);
        port.bootloader = info.isBootloader();
        ports.append(port);
    }
    return ports;
}

QList<SerialPortManager::Port> SerialPortManager::availablePorts()
{
    // Java USB enumeration leaks handles while a port is occupied. Preserve the last
    // snapshot instead of reporting an empty inventory and falsely declaring an unplug.
    if ((_singlePortOnly && anyPortReserved()) || (_scanTimer.isValid() && _scanTimer.elapsed() < 1000)) {
        return _ports;
    }
    _ports = _enumerator();
    _scanTimer.restart();
    QStringList identities;
    QStringList serialPorts;
    serialPorts.reserve(_ports.size());
    for (const Port& port : std::as_const(_ports)) {
        serialPorts.append(port.systemLocation);
        identities.append(port.systemLocation);
        identities.append(port.portName);
    }
    if (_serialPorts != serialPorts) {
        _serialPorts = std::move(serialPorts);
        emit serialPortsChanged();
    }
    emit portsEnumerated(identities);
    return _ports;
}

QString SerialPortManager::displayName(const QString& systemLocation)
{
    const QString name = systemLocation.trimmed();
    const auto ports = availablePorts();
    for (const auto& port : ports) {
        if (port.systemLocation == name || port.portName == name) {
            return port.displayName.isEmpty() ? port.portName : port.displayName;
        }
    }
    return {};
}

SerialPortManager::ReservationPtr SerialPortManager::reservePort(const QString& systemLocation)
{
    const QString port = systemLocation.trimmed();
    if (!canReservePort(port)) {
        return {};
    }
    for (auto it = _reservations.begin(); it != _reservations.end();) {
        it = it.value().expired() ? _reservations.erase(it) : std::next(it);
    }
    auto reservation = std::make_shared<const Reservation>(Reservation{port});
    _reservations.insert(port, reservation);
    return reservation;
}

bool SerialPortManager::isPortReserved(const QString& systemLocation) const
{
    return !_reservations.value(systemLocation.trimmed()).expired();
}

bool SerialPortManager::anyPortReserved() const
{
    for (const auto& reservation : _reservations) {
        if (!reservation.expired()) {
            return true;
        }
    }
    return false;
}

bool SerialPortManager::canReservePort(const QString& systemLocation) const
{
    return !systemLocation.trimmed().isEmpty() && !isPortReserved(systemLocation) &&
           !(_singlePortOnly && anyPortReserved());
}

SerialPortManager::ReservationPtr SerialPortManager::excludeFromAutoConnect(const QString& systemLocation)
{
    const QString port = systemLocation.trimmed();
    if (port.isEmpty()) {
        return {};
    }
    for (auto it = _autoConnectExclusions.begin(); it != _autoConnectExclusions.end();) {
        it = it.value().expired() ? _autoConnectExclusions.erase(it) : std::next(it);
    }
    if (auto existing = _autoConnectExclusions.value(port).lock()) {
        return existing;
    }
    auto exclusion = std::make_shared<const Reservation>(Reservation{port});
    _autoConnectExclusions.insert(port, exclusion);
    return exclusion;
}

bool SerialPortManager::canAutoConnectPort(const QString& systemLocation) const
{
    return !isAutoConnectExcluded(systemLocation) && canReservePort(systemLocation);
}

QStringList SerialPortManager::supportedBaudRates()
{
    static const QSet<qint32> kDefaultSupportedBaudRates = {
#ifdef Q_OS_UNIX
        50,     75,
#endif
        110,
#ifdef Q_OS_UNIX
        150,    200,    134,
#endif
        300,    600,    1200,
#ifdef Q_OS_UNIX
        1800,
#endif
        2400,   4800,   9600,
#ifdef Q_OS_WIN
        14400,
#endif
        19200,  38400,
#ifdef Q_OS_WIN
        56000,
#endif
        57600,  115200,
#ifdef Q_OS_WIN
        128000,
#endif
        230400,
#ifdef Q_OS_WIN
        256000,
#endif
        460800, 500000,
#ifdef Q_OS_LINUX
        576000,
#endif
        921600,
    };

    const QList<qint32> activeSupportedBaudRates = QSerialPortInfo::standardBaudRates();

    QSet<qint32> mergedBaudRateSet(kDefaultSupportedBaudRates.constBegin(), kDefaultSupportedBaudRates.constEnd());
    (void) mergedBaudRateSet.unite(
        QSet<qint32>(activeSupportedBaudRates.constBegin(), activeSupportedBaudRates.constEnd()));

    QList<qint32> mergedBaudRateList = mergedBaudRateSet.values();
    std::sort(mergedBaudRateList.begin(), mergedBaudRateList.end());

    QStringList supportBaudRateStrings{};
    supportBaudRateStrings.reserve(mergedBaudRateList.size());
    for (const qint32 rate : std::as_const(mergedBaudRateList)) {
        supportBaudRateStrings.append(QString::number(rate));
    }

    return supportBaudRateStrings;
}

bool SerialPortManager::isAutoConnectExcluded(const QString& systemLocation) const
{
    return !_autoConnectExclusions.value(systemLocation.trimmed()).expired();
}
