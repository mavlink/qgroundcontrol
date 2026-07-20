#pragma once

#include <QtCore/QElapsedTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include <memory>

#include "LinkConfiguration.h"
#include "MAVLinkMessageType.h"

class LinkManager;
class SigningController;

/// \brief The link interface defines the interface for all links used to communicate with the ground station application.
///
class LinkInterface : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    friend class LinkManager;

public:
    virtual ~LinkInterface();

    Q_INVOKABLE virtual void disconnect() = 0; // Implementations should guard against multiple calls

    virtual bool isConnected() const = 0;
    virtual bool isLogReplay() const { return false; }
    virtual bool isSecureConnection() const { return false; } ///< Returns true if the connection is secure (e.g. USB, wired ethernet)

    SharedLinkConfigurationPtr linkConfiguration() { return _config; }
    const SharedLinkConfigurationPtr linkConfiguration() const { return _config; }
    uint8_t mavlinkChannel() const;
    bool mavlinkChannelIsSet() const;
    bool decodedFirstMavlinkPacket() const { return _decodedFirstMavlinkPacket; }
    void setDecodedFirstMavlinkPacket(bool decodedFirstMavlinkPacket) { _decodedFirstMavlinkPacket = decodedFirstMavlinkPacket; }
    void writeBytesThreadSafe(const char *bytes, int length);
    /// Single message-level send chokepoint: re-signs (if signing is active), serializes, then writes. All
    /// outbound mavlink_message_t sends must route through here so signing can't be bypassed.
    void sendMessageThreadSafe(mavlink_message_t &message);
    void addVehicleReference() { ++_vehicleReferenceCount; }
    void removeVehicleReference();
    /// Called for each received v1 message which QGC drops. The warning is deferred by a grace
    /// period since ArduPilot starts links in v1 and upgrades to v2 on first v2 message from QGC.
    void reportMavlinkV1Traffic();
    /// Called when a v2 message is received: permanently suppresses the v1-only warning for this link.
    void reportMavlinkV2Traffic() { _mavlinkV2TrafficSeen = true; }
    bool mavlinkV1TrafficReported() const { return _mavlinkV1TrafficReported; }

    /// Grace period a link is given to upgrade from MAVLink v1 to v2 (ArduPilot starts out in v1)
    /// before the v1-only warning is reported. Settable for tests.
    static constexpr int kMavlinkV1TrafficGraceMsecsDefault = 10000;
    static void setMavlinkV1TrafficGraceMsecs(int msecs) { _mavlinkV1TrafficGraceMsecs = msecs; }

    /// Per-link signing state and confirmation state machine. Non-null after channel allocation.
    SigningController* signing() { return _signingController.get(); }
    const SigningController* signing() const { return _signingController.get(); }

signals:
    void bytesReceived(LinkInterface *link, const QByteArray &data);
    void bytesSent(LinkInterface *link, const QByteArray &data);
    void connected();
    void disconnected();
    void communicationError(const QString &title, const QString &error);

protected:
    /// Links are only created by LinkManager so constructor is not public
    explicit LinkInterface(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);

    /// Called by the LinkManager during LinkInterface construction instructing the link to setup channels.
    /// Default implementation allocates a single channel. But some link types (such as MockLink) need more than one.
    virtual bool _allocateMavlinkChannel();

    virtual void _freeMavlinkChannel();

    void _connectionRemoved();

    SharedLinkConfigurationPtr _config;

private slots:
    /// Not thread safe if called directly, only writeBytesThreadSafe is thread safe
    virtual void _writeBytes(const QByteArray &bytes) = 0;

private:
    /// connect is private since all links should be created through LinkManager::createConnectedLink calls
    virtual bool _connect() = 0;

    uint8_t _mavlinkChannel = std::numeric_limits<uint8_t>::max();
    bool _decodedFirstMavlinkPacket = false;
    int _vehicleReferenceCount = 0;
    bool _mavlinkV1TrafficReported = false;
    bool _mavlinkV2TrafficSeen = false;
    QElapsedTimer _mavlinkV1FirstSeenTimer;
    static inline int _mavlinkV1TrafficGraceMsecs = kMavlinkV1TrafficGraceMsecsDefault;
    /// Must `reset()` in `_freeMavlinkChannel` before LinkManager frees the channel so the
    /// controller can flush the final timestamp.
    std::unique_ptr<SigningController> _signingController;
};

typedef std::shared_ptr<LinkInterface> SharedLinkInterfacePtr;
typedef std::weak_ptr<LinkInterface> WeakLinkInterfacePtr;
