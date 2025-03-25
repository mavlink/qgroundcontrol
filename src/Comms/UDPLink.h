/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#endif
#include <dns_sd.h>
#endif

#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QUdpSocket;
class QThread;

Q_DECLARE_LOGGING_CATEGORY(UDPLinkLog)

/*===========================================================================*/

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

/*===========================================================================*/

class UDPConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QStringList hostList READ hostList NOTIFY hostListChanged)
    Q_PROPERTY(quint16 localPort READ localPort WRITE setLocalPort NOTIFY localPortChanged)

public:
    explicit UDPConfiguration(const QString &name, QObject *parent = nullptr);
    explicit UDPConfiguration(const UDPConfiguration *source, QObject *parent = nullptr);
    virtual ~UDPConfiguration();

    Q_INVOKABLE void addHost(const QString &host);
    Q_INVOKABLE void addHost(const QString &host, quint16 port);
    Q_INVOKABLE void removeHost(const QString &host);
    Q_INVOKABLE void removeHost(const QString &host, quint16 port);

    LinkType type() const override { return LinkConfiguration::TypeUdp; }
    void setAutoConnect(bool autoc = true) override;
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) const override;
    QString settingsURL() const override { return QStringLiteral("UdpSettings.qml"); }
    QString settingsTitle() const override { return tr("UDP Link Settings"); }

    QStringList hostList() const { return _hostList; }
    QList<std::shared_ptr<UDPClient>> targetHosts() const { return _targetHosts; }
    quint16 localPort() const { return _localPort; }
    void setLocalPort(quint16 port) { if (port != _localPort) { _localPort = port; emit localPortChanged(); } }

signals:
    void hostListChanged();
    void localPortChanged();

private:
    void _updateHostList();

    static QString _getIpAddress(const QString &address);

    QStringList _hostList;
    QList<std::shared_ptr<UDPClient>> _targetHosts;
    quint16 _localPort = 0;
};

/*===========================================================================*/

class UDPWorker : public QObject
{
    Q_OBJECT

public:
    explicit UDPWorker(const UDPConfiguration *config, QObject *parent = nullptr);
    virtual ~UDPWorker();

    bool isConnected() const;

public slots:
    void setupSocket();
    void connectLink();
    void disconnectLink();
    void writeData(const QByteArray &data);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);

private slots:
    void _onSocketConnected();
    void _onSocketDisconnected();
    void _onSocketReadyRead();
    void _onSocketBytesWritten(qint64 bytes);
    void _onSocketErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    const UDPConfiguration *_udpConfig = nullptr;
    QUdpSocket *_socket = nullptr;
    QMutex _sessionTargetsMutex;
    QList<std::shared_ptr<UDPClient>> _sessionTargets;
    bool _isConnected = false;
    bool _errorEmitted = false;
    QSet<QHostAddress> _localAddresses;

    static const QHostAddress _multicastGroup;

#ifdef QGC_ZEROCONF_ENABLED
    void _registerZeroconf(uint16_t port);
    void _deregisterZeroconf();
    static void _zeroconfRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context);

    DNSServiceRef _dnssServiceRef = nullptr;
#endif
};

/*===========================================================================*/

class UDPLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit UDPLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~UDPLink();

    bool isConnected() const override;
    void disconnect() override;
    bool isSecureConnection() const override;

private slots:
    void _writeBytes(const QByteArray &data) override;
    void _onConnected();
    void _onDisconnected();
    void _onErrorOccurred(const QString &errorString);
    void _onDataReceived(const QByteArray &data);
    void _onDataSent(const QByteArray &data);

private:
    bool _connect() override;

    const UDPConfiguration *_udpConfig = nullptr;
    UDPWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;
};
