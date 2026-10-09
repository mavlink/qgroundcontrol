#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QGCSerialPortInfo.h"

/// Shared serial inventory and exclusive claims. Called on the application thread.
class SerialPortManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Managed by QGroundControl")

    Q_PROPERTY(QStringList serialPorts READ serialPorts NOTIFY serialPortsChanged)
    Q_PROPERTY(QStringList serialBaudRates READ supportedBaudRates CONSTANT)

public:
    struct Port
    {
        QString systemLocation;
        QString portName;
        QGCSerialPortInfo::BoardType_t boardType = QGCSerialPortInfo::BoardTypeUnknown;
        QString boardName;
        bool bootloader = false;
        /// Shared by interfaces with the same VID/PID/serial; empty when identity is unavailable.
        QString physicalDeviceId = {};
        QString description = {};
        QString displayName = {};
    };

    struct Reservation
    {
        QString systemLocation;
    };

    using ReservationPtr = std::shared_ptr<const Reservation>;
    using Enumerator = std::function<QList<Port>()>;

    /// How long a newly listed port settles before automatic connection opens it.
#ifdef Q_OS_WIN
    // Allow the bootloader to finish before opening a new Windows device.
    static constexpr std::chrono::milliseconds NEW_PORT_SETTLE_DELAY{6000};
#else
    static constexpr std::chrono::milliseconds NEW_PORT_SETTLE_DELAY{1000};
#endif

    explicit SerialPortManager(QObject* parent = nullptr, Enumerator enumerator = {});
    static SerialPortManager* instance();

    /// Whether @a port is the first interface listed of its physical device, which this records in @a seenDevices.
    /// A port without a device identity is always primary.
    static bool isPrimaryInterface(const Port& port, QSet<QString>& seenDevices);

    QList<Port> availablePorts();
    /// As availablePorts(), without reusing a scan made less than a second ago.
    void rescan();
    QString displayName(const QString& systemLocation);

    QStringList serialPorts() const { return _serialPorts; }

    static QStringList supportedBaudRates();
    ReservationPtr reservePort(const QString& systemLocation);
    bool canReservePort(const QString& systemLocation) const;
    bool isPortReserved(const QString& systemLocation) const;
    bool anyPortReserved() const;

    /// Routing exclusions survive reconnects without marking the hardware occupied.
    ReservationPtr excludeFromAutoConnect(const QString& systemLocation);
    bool canAutoConnectPort(const QString& systemLocation) const;
    bool isAutoConnectExcluded(const QString& systemLocation) const;

    void setSinglePortOnly(bool enabled) { _singlePortOnly = enabled; }

signals:
    void serialPortsChanged();
    /// Emitted only after a fresh scan, never for a retained Android snapshot. Includes native names and paths.
    void portsEnumerated(const QStringList& availablePorts);

private:
    static QList<Port> _enumeratePorts();
    Enumerator _enumerator;
    QList<Port> _ports;
    QStringList _serialPorts;
    QElapsedTimer _scanTimer;
    QHash<QString, std::weak_ptr<const Reservation>> _reservations;
    QHash<QString, std::weak_ptr<const Reservation>> _autoConnectExclusions;
    bool _singlePortOnly = false;
};

/// Ports a scan lists for the first time wait SerialPortManager::NEW_PORT_SETTLE_DELAY before automatic connection
/// opens them. Times are microseconds of one monotonic clock.
class SerialPortSettleTracker
{
    friend class LinkManagerTest;
    friend class SerialAutoConnectTest;

public:
    /// Records @a port as listed at @a nowUs; true once it has been listed for the settle delay.
    bool settled(const QString& port, quint64 nowUs)
    {
        const auto it = _deadlinesUs.constFind(port);
        if (it == _deadlinesUs.cend()) {
            _deadlinesUs.insert(port, nowUs + std::chrono::duration_cast<std::chrono::microseconds>(
                                                  SerialPortManager::NEW_PORT_SETTLE_DELAY)
                                                  .count());
            return false;
        }
        return nowUs >= *it;
    }

    bool contains(const QString& port) const { return _deadlinesUs.contains(port); }

    bool isEmpty() const { return _deadlinesUs.isEmpty(); }

    void remove(const QString& port) { _deadlinesUs.remove(port); }

    /// Forgets the ports @a forget returns true for.
    template <typename Predicate>
    void removeIf(Predicate forget)
    {
        _deadlinesUs.removeIf([&forget](const auto& it) { return forget(it.key()); });
    }

    void clear() { _deadlinesUs.clear(); }

private:
    QMap<QString, quint64> _deadlinesUs;
};
