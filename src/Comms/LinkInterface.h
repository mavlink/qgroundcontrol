/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QThread>
#include <QtCore/QLoggingCategory>

#include "LinkConfiguration.h"

class LinkManager;

Q_DECLARE_LOGGING_CATEGORY(LinkInterfaceLog)

/// The link interface defines the interface for all links used to communicate with the ground station application.
class LinkInterface : public QThread
{
    Q_OBJECT

    Q_PROPERTY(bool isPX4Flow  READ isPX4Flow  CONSTANT)

    friend class LinkManager;

public:
    virtual ~LinkInterface();

    Q_INVOKABLE virtual void disconnect() = 0;

    virtual bool isConnected() const = 0;
    virtual bool isLogReplay() { return false; }
    virtual bool isSecureConnection() { return false; } ///< Returns true if the connection is secure (e.g. USB, wired ethernet)

    SharedLinkConfigurationPtr linkConfiguration() { return _config; }
    uint8_t mavlinkChannel() const;
    bool mavlinkChannelIsSet() const;
    bool isPX4Flow() const { return _isPX4Flow; }
    bool decodedFirstMavlinkPacket(void) const { return _decodedFirstMavlinkPacket; }
    void setDecodedFirstMavlinkPacket(bool decodedFirstMavlinkPacket) { _decodedFirstMavlinkPacket = decodedFirstMavlinkPacket; }
    void writeBytesThreadSafe(const char *bytes, int length);
    void addVehicleReference() { ++_vehicleReferenceCount; }
    void removeVehicleReference();
    bool initMavlinkSigning();
    void setSigningSignatureFailure(bool failure);

signals:
    void bytesReceived(LinkInterface *link, const QByteArray &data);
    void bytesSent(LinkInterface *link, const QByteArray &data);
    void connected();
    void disconnected();
    void communicationError(const QString &title, const QString &error);

protected:
    /// Links are only created by LinkManager so constructor is not public
    LinkInterface(SharedLinkConfigurationPtr &config, bool isPX4Flow = false, QObject *parent = nullptr);

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
    bool _isPX4Flow = false;
    int _vehicleReferenceCount = 0;
    bool _signingSignatureFailure = false;
};

typedef std::shared_ptr<LinkInterface> SharedLinkInterfacePtr;
typedef std::weak_ptr<LinkInterface> WeakLinkInterfacePtr;
