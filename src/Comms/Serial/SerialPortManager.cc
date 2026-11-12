#include "SerialPortManager.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QSet>

#include <algorithm>
#include <iterator>
#include <utility>

#include "QGCLoggingCategory.h"

#ifdef Q_OS_ANDROID
#include "AndroidSerial.h"
#endif

QGC_LOGGING_CATEGORY(SerialPortManagerLog, "Comms.Serial.SerialPortManager")
Q_APPLICATION_STATIC(SerialPortManager, _serialPortManager)

SerialPortManager::SerialPortManager(QObject* parent, Enumerator enumerator)
    : QObject(parent), _enumerator(enumerator ? std::move(enumerator) : &_enumeratePorts)
{
#ifdef Q_OS_ANDROID
    // Android tells when a USB device is attached, detached or permitted, so the inventory does not wait for the
    // next caller to ask for it.
    (void) connect(AndroidSerial::DeviceWatcher::instance(), &AndroidSerial::DeviceWatcher::devicesChanged, this,
                   &SerialPortManager::rescan);
#endif
}

SerialPortManager* SerialPortManager::instance()
{
    return _serialPortManager();
}

bool SerialPortManager::isPrimaryInterface(const Port& port, QSet<QString>& seenDevices)
{
    if (port.physicalDeviceId.isEmpty()) {
        return true;
    }
    const qsizetype seen = seenDevices.size();
    seenDevices.insert(port.physicalDeviceId);
    return seenDevices.size() != seen;
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
        port.stableName = info.stableName();
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
    if (_scanTimer.isValid() && _scanTimer.elapsed() < 1000) {
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
        if (!port.stableName.isEmpty()) {
            identities.append(port.stableName);
        }
    }
    if (_serialPorts != serialPorts) {
        _serialPorts = std::move(serialPorts);
        emit serialPortsChanged();
    }
    emit portsEnumerated(identities);
    return _ports;
}

void SerialPortManager::rescan()
{
    _scanTimer.invalidate();
    (void) availablePorts();
}

QString SerialPortManager::displayName(const QString& systemLocation)
{
    (void) availablePorts();
    const Port* const port = _portNamed(systemLocation.trimmed());
    if (!port) {
        return {};
    }
    return port->displayName.isEmpty() ? port->portName : port->displayName;
}

SerialPortManager::ReservationPtr SerialPortManager::reservePort(const QString& systemLocation, const QString& owner)
{
    const QString port = systemLocation.trimmed();
    if (!canReservePort(port)) {
        return {};
    }
    for (auto it = _reservations.begin(); it != _reservations.end();) {
        it = it.value().expired() ? _reservations.erase(it) : std::next(it);
    }
    auto reservation = std::make_shared<const Reservation>(Reservation{.systemLocation = port, .owner = owner});
    _reservations.insert(port, reservation);
    return reservation;
}

const SerialPortManager::Port* SerialPortManager::_portNamed(const QString& name) const
{
    const Port* found = nullptr;
    for (const Port& port : _ports) {
        if (!port.hasName(name)) {
            continue;
        }
        if (found) {
            return nullptr;
        }
        found = &port;
    }
    return found;
}

QString SerialPortManager::_systemLocation(const QString& name) const
{
    const QString trimmed = name.trimmed();
    const Port* const port = _portNamed(trimmed);
    return port ? port->systemLocation : trimmed;
}

SerialPortManager::ReservationPtr SerialPortManager::_reservationOf(const QString& name) const
{
    // Compared by what the names stand for now: a stable name stands for another system location once its
    // device has been replugged.
    const QString location = _systemLocation(name);
    for (auto it = _reservations.cbegin(); it != _reservations.cend(); ++it) {
        ReservationPtr reservation = it.value().lock();
        if (reservation && _systemLocation(it.key()) == location) {
            return reservation;
        }
    }
    return {};
}

bool SerialPortManager::isPortReserved(const QString& systemLocation) const
{
    return _reservationOf(systemLocation) != nullptr;
}

QString SerialPortManager::reservationOwner(const QString& systemLocation) const
{
    const ReservationPtr reservation = _reservationOf(systemLocation);
    return reservation ? reservation->owner : QString();
}

bool SerialPortManager::isSamePort(const QString& first, const QString& second) const
{
    return _systemLocation(first) == _systemLocation(second);
}

QString SerialPortManager::persistentName(const QString& name) const
{
    QString trimmed = name.trimmed();
    for (const Port& port : _ports) {
        if (port.systemLocation != trimmed && port.portName != trimmed) {
            continue;
        }
        const auto sharesStableName = [&port](const Port& other) { return other.stableName == port.stableName; };
        const bool findsOnlyThisPort =
            !port.stableName.isEmpty() && std::count_if(_ports.cbegin(), _ports.cend(), sharesStableName) == 1;
        return findsOnlyThisPort ? port.stableName : port.systemLocation;
    }
    return trimmed;
}

bool SerialPortManager::canReservePort(const QString& systemLocation) const
{
    return !systemLocation.trimmed().isEmpty() && !isPortReserved(systemLocation);
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
