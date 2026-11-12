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

    friend class LinkManagerTest;

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
        /// Finds the port again once its device has been replugged, which gives it another system location. Empty
        /// where the platform has no such name. Devices of a kind that report the same serial number, or none,
        /// share theirs.
        QString stableName = {};

        /// Whether @a name is the port's system location, port name or stable name.
        bool hasName(const QString& name) const
        {
            return !name.isEmpty() && (name == systemLocation || name == portName || name == stableName);
        }
    };

    struct Reservation
    {
        QString systemLocation;
        /// What holds the port, in words for the user; empty when the holder gave none.
        QString owner = {};
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
    ReservationPtr reservePort(const QString& systemLocation, const QString& owner = {});
    bool canReservePort(const QString& systemLocation) const;
    bool isPortReserved(const QString& systemLocation) const;
    /// The owner the port is reserved with; empty when it is not reserved or its holder gave none.
    QString reservationOwner(const QString& systemLocation) const;
    /// Whether both names are of one listed port, or are the same name.
    bool isSamePort(const QString& first, const QString& second) const;
    /// The name for a saved configuration to keep of the listed port with this system location or port name: the
    /// port's stable name if it has one that no other listed port shares, else its system location. Any other
    /// name is returned as it is.
    QString persistentName(const QString& name) const;

    /// Routing exclusions survive reconnects without marking the hardware occupied.
    ReservationPtr excludeFromAutoConnect(const QString& systemLocation);
    bool canAutoConnectPort(const QString& systemLocation) const;
    bool isAutoConnectExcluded(const QString& systemLocation) const;

signals:
    void serialPortsChanged();
    /// Emitted only after a fresh scan, with every name of the ports it lists.
    void portsEnumerated(const QStringList& availablePorts);

private:
    static QList<Port> _enumeratePorts();
    /// The only listed port with this name, or null. A stable name that several listed ports share stands for
    /// none of them: the first one found could be on another vehicle than the one that was meant.
    const Port* _portNamed(const QString& name) const;
    /// The system location of the listed port with this name, or the name itself if it stands for no listed port.
    QString _systemLocation(const QString& name) const;
    /// The live reservation of the port with this name, under whichever of the port's names it was made.
    ReservationPtr _reservationOf(const QString& name) const;
    Enumerator _enumerator;
    QList<Port> _ports;
    QStringList _serialPorts;
    QElapsedTimer _scanTimer;
    QHash<QString, std::weak_ptr<const Reservation>> _reservations;
    QHash<QString, std::weak_ptr<const Reservation>> _autoConnectExclusions;
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
