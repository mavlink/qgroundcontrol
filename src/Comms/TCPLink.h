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
#include <QtNetwork/QHostAddress>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QTcpSocket;

Q_DECLARE_LOGGING_CATEGORY(TCPLinkLog)

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

    QString host() const { return _host.toString(); }
    void setHost(const QString &host) { if (host != _host.toString()) { _host.setAddress(host); emit hostChanged(); } }
    quint16 port() const { return _port; }
    void setPort(quint16 port) { if (port != _port) { _port = port; emit portChanged(); } }

    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) override;
    QString settingsURL() override { return QStringLiteral("TcpSettings.qml"); }
    QString settingsTitle() override { return tr("TCP Link Settings"); }

signals:
    void hostChanged();
    void portChanged();

private:
    QHostAddress _host;
    quint16 _port = 5760;
};

////////////////////////////////////////////////////////////////////

class TCPLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit TCPLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~TCPLink();

    void run() override {};
    bool isConnected() const override;
    void disconnect() override;
    bool isSecureConnection() override;

private slots:
    void _writeBytes(const QByteArray &bytes) override;
    void _readBytes();

private:
    bool _connect() override;

    const TCPConfiguration *_tcpConfig = nullptr;
    QTcpSocket *_socket = nullptr;
};
