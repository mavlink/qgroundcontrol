#pragma once
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <memory>

class GPSSerialDiscovery : public QObject
{
    Q_OBJECT
public:
    struct Port
    {
        QString systemLocation;
        QString portName;
        QString boardName;
        bool receiver = false;
        bool bootloader = false;
        bool autoConnectAllowed = true;
    };

    struct Lease
    {
        virtual ~Lease() = default;
    };

    using ReservationPtr = std::shared_ptr<const Lease>;
    explicit GPSSerialDiscovery(QObject* parent = nullptr);
    ~GPSSerialDiscovery() override;
    virtual QList<Port> availablePorts() = 0;
    virtual ReservationPtr reservePort(const QString& device) = 0;
    virtual ReservationPtr excludeFromAutoConnect(const QString& device) = 0;
    virtual bool canReservePort(const QString& device) const = 0;
    virtual bool canAutoConnectPort(const QString& device) const = 0;
    virtual bool isAutoConnectExcluded(const QString& device) const = 0;
signals:
    void serialPortsChanged();
};
