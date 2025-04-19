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
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostAddress>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QTcpSocket;
class QThread;

Q_DECLARE_LOGGING_CATEGORY(TCPLinkLog)

/*===========================================================================*/

class TCPConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)

public:
    explicit TCPConfiguration(const QString &name, QObject *parent = nullptr);
    explicit TCPConfiguration(const TCPConfiguration *copy, QObject *parent = nullptr);
    virtual ~TCPConfiguration();

    LinkType type() const override { return LinkConfiguration::TypeTcp; }
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) const override;
    QString settingsURL() const override { return QStringLiteral("TcpSettings.qml"); }
    QString settingsTitle() const override { return tr("TCP Link Settings"); }

    QString host() const { return _host.toString(); }
    void setHost(const QString &host) { if (host != _host.toString()) { _host.setAddress(host); emit hostChanged(); } }
    quint16 port() const { return _port; }
    void setPort(quint16 port) { if (port != _port) { _port = port; emit portChanged(); } }

signals:
    void hostChanged();
    void portChanged();

private:
    QHostAddress _host;
    quint16 _port = 5760;
};

/*===========================================================================*/

class TCPWorker : public QObject
{
    Q_OBJECT

public:
    explicit TCPWorker(const TCPConfiguration *config, QObject *parent = nullptr);
    ~TCPWorker();

    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);

public slots:
    void setupSocket();
    void connectToHost();
    void disconnectFromHost();
    void writeData(const QByteArray &data);

private slots:
    void _onSocketConnected();
    void _onSocketDisconnected();
    void _onSocketReadyRead();
    void _onSocketBytesWritten(qint64 bytes);
    void _onSocketErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    const TCPConfiguration *_config = nullptr;
    QTcpSocket *_socket = nullptr;
    bool _errorEmitted = false;
};

/*===========================================================================*/

class TCPLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit TCPLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~TCPLink();

    bool isConnected() const override;
    void disconnect() override;
    bool isSecureConnection() const override;

private slots:
    void _writeBytes(const QByteArray &bytes) override;
    void _onConnected();
    void _onDisconnected();
    void _onErrorOccurred(const QString &errorString);
    void _onDataReceived(const QByteArray &data);
    void _onDataSent(const QByteArray &data);

private:
    bool _connect() override;

    const TCPConfiguration *_tcpConfig = nullptr;
    TCPWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;
};
