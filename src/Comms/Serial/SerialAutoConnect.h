#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QMap>

#include <functional>

#include "LinkConfiguration.h"
#include "SerialPortManager.h"

/// Application-thread MAVLink serial discovery. LinkManager owns creation and live links.
class SerialAutoConnect
{
    friend class SerialAutoConnectTest;
    friend class LinkManagerTest;

public:
    struct Options
    {
        bool pixhawk = false;
        bool sikRadio = false;
        bool openPilot = false;
    };

    using Connect = std::function<void(SharedLinkConfigurationPtr&)>;
    SerialAutoConnect(SerialPortManager& ports, Connect connect);
    ~SerialAutoConnect();
    void update(const QList<SerialPortManager::Port>& ports, const Options& options);

private:
    static bool _allowed(QGCSerialPortInfo::BoardType_t boardType, const Options& options);

    struct Identity
    {
        QString physicalDeviceId;
        QGCSerialPortInfo::BoardType_t boardType;
        bool bootloader;
        bool operator==(const Identity&) const = default;
    };

    SerialPortManager& _ports;
    Connect _connect;
    QMap<QString, Identity> _identities;
    QMap<QString, QDeadlineTimer> _waitingPorts;
    QMap<QString, SharedLinkConfigurationPtr> _configs;
#ifdef Q_OS_WIN
    // Allow the bootloader to finish before opening a new Windows device.
    static constexpr int kConnectDelayMs = 6000;
#else
    static constexpr int kConnectDelayMs = 1000;
#endif
};
