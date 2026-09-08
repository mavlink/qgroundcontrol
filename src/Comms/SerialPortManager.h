#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

#include <functional>
#include <memory>

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
        bool autoConnectAllowed = true;
    };

    struct Reservation
    {
        QString systemLocation;
    };

    using ReservationPtr = std::shared_ptr<const Reservation>;
    using Enumerator = std::function<QList<Port>()>;

    explicit SerialPortManager(QObject* parent = nullptr, Enumerator enumerator = {});
    static SerialPortManager* instance();

    QList<Port> availablePorts();

    QStringList serialPorts() const { return _serialPorts; }

    static QStringList supportedBaudRates();
    ReservationPtr reservePort(const QString& systemLocation);
    bool canReservePort(const QString& systemLocation) const;
    bool isPortReserved(const QString& systemLocation) const;
    bool anyPortReserved() const;

    /// Routing exclusions survive reconnects without marking the hardware occupied.
    ReservationPtr excludeFromAutoConnect(const QString& systemLocation);
    bool canAutoConnectPort(const QString& systemLocation) const;

    void setSinglePortOnly(bool enabled) { _singlePortOnly = enabled; }

signals:
    void serialPortsChanged();

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
