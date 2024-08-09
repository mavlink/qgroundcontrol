/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtNetwork/QHostAddress>

#ifdef QGC_ZEROCONF_ENABLED
#include <dns_sd.h>
#endif

#include "LinkConfiguration.h"
#include "LinkInterface.h"

Q_DECLARE_LOGGING_CATEGORY(UDPLinkLog)

class QUdpSocket;

struct UDPClient
{
    UDPClient(const QHostAddress &address, quint16 port)
        : address(address)
        , port(port)
    {}

    explicit UDPClient(const UDPClient *other)
        : address(other->address)
        , port(other->port)
    {}

    bool operator==(const UDPClient &other) const
    {
        return ((address == other.address) && (port == other.port));
    }

    UDPClient &operator=(const UDPClient &other)
    {
        address = other.address;
        port = other.port;

        return *this;
    }

    QHostAddress address;
    quint16 port = 0;
};

class UDPConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QStringList hostList  READ hostList                     NOTIFY hostListChanged)
    Q_PROPERTY(quint16     localPort READ localPort WRITE setLocalPort NOTIFY localPortChanged)

public:
    explicit UDPConfiguration(const QString &name, QObject *parent = nullptr);
    explicit UDPConfiguration(UDPConfiguration *copy, QObject *parent = nullptr);
    virtual ~UDPConfiguration();

    Q_INVOKABLE void addHost(const QString &host);
    Q_INVOKABLE void addHost(const QString &host, quint16 port);
    Q_INVOKABLE void removeHost(const QString &host);

    quint16 localPort() const { return _localPort; }
    void setLocalPort(quint16 port);

    QStringList hostList() const { return _hostList; }
    const QList<std::shared_ptr<UDPClient>> targetHosts() const { return _targetHosts; }

    LinkType type() override { return LinkConfiguration::TypeUdp; }
    void copyFrom(LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) override;
    QString settingsURL() override { return QStringLiteral("UdpSettings.qml"); }
    QString settingsTitle() override { return QStringLiteral("UDP Link Settings"); }

signals:
    void hostListChanged();
    void localPortChanged();

private:
    void _updateHostList();

    static QString _getIpAddress(const QString &address);

    QStringList _hostList;
    quint16 _localPort = 0;
    QList<std::shared_ptr<UDPClient>> _targetHosts;
};

class UDPLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit UDPLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~UDPLink();

    void run() override {};
    bool isConnected() const override;
    void disconnect() override;

private slots:
    void _writeBytes(const QByteArray &data) override;
    void _readBytes();

private:
    bool _connect() override;
    bool _isIpLocal(const QHostAddress &address) const;
#ifdef QGC_ZEROCONF_ENABLED
    void _registerZeroconf(uint16_t port, const char *regType);
    void _deregisterZeroconf();
#endif

    const QList<QHostAddress> _localAddresses;
    QMutex _sessionTargetsMutex;
    QList<std::shared_ptr<UDPClient>> _sessionTargets;
#ifdef QGC_ZEROCONF_ENABLED
    DNSServiceRef _dnssServiceRef = nullptr;
#endif
    const UDPConfiguration *_udpConfig = nullptr;
    QUdpSocket *_socket = nullptr;
};
