#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>

#include <functional>
#include <memory>

#include "QGCSerialPortInfo.h"

/// Shared serial inventory and exclusive claims. Called on the application thread.
class SerialPortManager : public QObject
{
    Q_OBJECT

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
    ReservationPtr reservePort(const QString& systemLocation);
    bool canReservePort(const QString& systemLocation) const;
    bool isPortReserved(const QString& systemLocation) const;
    bool anyPortReserved() const;

    void setSinglePortOnly(bool enabled) { _singlePortOnly = enabled; }

private:
    static QList<Port> _enumeratePorts();
    Enumerator _enumerator;
    QList<Port> _ports;
    QElapsedTimer _scanTimer;
    QHash<QString, std::weak_ptr<const Reservation>> _reservations;
    bool _singlePortOnly = false;
};
