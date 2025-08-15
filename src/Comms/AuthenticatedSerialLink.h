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
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include "LinkConfiguration.h"
#include "LinkInterface.h"
#include "QGCSerialPortInfo.h"

class QThread;

Q_DECLARE_LOGGING_CATEGORY(AuthenticatedSerialLinkLog)

/*===========================================================================*/

class AuthenticatedSerialConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(qint32                   baud            READ baud            WRITE setBaud        NOTIFY baudChanged)
    Q_PROPERTY(QSerialPort::DataBits    dataBits        READ dataBits        WRITE setDataBits    NOTIFY dataBitsChanged)
    Q_PROPERTY(QSerialPort::FlowControl flowControl     READ flowControl     WRITE setFlowControl NOTIFY flowControlChanged)
    Q_PROPERTY(QSerialPort::StopBits    stopBits        READ stopBits        WRITE setStopBits    NOTIFY stopBitsChanged)
    Q_PROPERTY(QSerialPort::Parity      parity          READ parity          WRITE setParity      NOTIFY parityChanged)
    Q_PROPERTY(QString                  portName        READ portName        WRITE setPortName    NOTIFY portNameChanged)
    Q_PROPERTY(QString                  portDisplayName READ portDisplayName                      NOTIFY portDisplayNameChanged)
    Q_PROPERTY(bool                     usbDirect       READ usbDirect       WRITE setUsbDirect   NOTIFY usbDirectChanged)
    Q_PROPERTY(QString                  username        READ username        WRITE setUsername    NOTIFY usernameChanged)
    Q_PROPERTY(QString                  password        READ password        WRITE setPassword    NOTIFY passwordChanged)
    Q_PROPERTY(QString                  authHost        READ authHost        WRITE setAuthHost    NOTIFY authHostChanged)
    Q_PROPERTY(quint16                  authPort        READ authPort        WRITE setAuthPort    NOTIFY authPortChanged)
    Q_PROPERTY(QString                  dataHost        READ dataHost        WRITE setDataHost    NOTIFY dataHostChanged)
    Q_PROPERTY(quint16                  dataPort        READ dataPort        WRITE setDataPort    NOTIFY dataPortChanged)
    Q_PROPERTY(QString                  authUrl         READ authUrl         WRITE setAuthUrl     NOTIFY authUrlChanged)

public:
    explicit AuthenticatedSerialConfiguration(const QString &name, QObject *parent = nullptr);
    explicit AuthenticatedSerialConfiguration(const AuthenticatedSerialConfiguration *copy, QObject *parent = nullptr);
    virtual ~AuthenticatedSerialConfiguration();

    LinkType type() const override { return LinkConfiguration::TypeAuthenticatedSerial; }
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) const override;
    QString settingsURL() const override { return QStringLiteral("AuthenticatedSerialSettings.qml"); }
    QString settingsTitle() const override { return tr("Authenticated Serial Link Settings"); }

    qint32 baud() const { return _baud; }
    void setBaud(qint32 baud) { if (baud != _baud) { _baud = baud; emit baudChanged(); } }

    QSerialPort::DataBits dataBits() const { return _dataBits; }
    void setDataBits(QSerialPort::DataBits databits) { if (databits != _dataBits) { _dataBits = databits; emit dataBitsChanged(); } }

    QSerialPort::FlowControl flowControl() const { return _flowControl; }
    void setFlowControl(QSerialPort::FlowControl flowControl) { if (flowControl != _flowControl) { _flowControl = flowControl; emit flowControlChanged(); } }

    QSerialPort::StopBits stopBits() const { return _stopBits; }
    void setStopBits(QSerialPort::StopBits stopBits) { if (stopBits != _stopBits) { _stopBits = stopBits; emit stopBitsChanged(); } }

    QSerialPort::Parity parity() const { return _parity; }
    void setParity(QSerialPort::Parity parity) { if (parity != _parity) { _parity = parity; emit parityChanged(); } }

    QString portName() const { return _portName; }
    void setPortName(const QString &name);

    QString portDisplayName() const { return _portDisplayName; }
    void setPortDisplayName(const QString &portDisplayName) { if (portDisplayName != _portDisplayName) { _portDisplayName = portDisplayName; emit portDisplayNameChanged(); } }

    bool usbDirect() const { return _usbDirect; }
    void setUsbDirect(bool usbDirect) { if (usbDirect != _usbDirect) { _usbDirect = usbDirect; emit usbDirectChanged(); } }

    QString username() const { return _username; }
    void setUsername(const QString &username) { if (username != _username) { _username = username; emit usernameChanged(); } }

    QString password() const { return _password; }
    void setPassword(const QString &password) { if (password != _password) { _password = password; emit passwordChanged(); } }

    QString authHost() const { return _authHost; }
    void setAuthHost(const QString &host) { if (host != _authHost) { _authHost = host; emit authHostChanged(); } }

    quint16 authPort() const { return _authPort; }
    void setAuthPort(quint16 port) { if (port != _authPort) { _authPort = port; emit authPortChanged(); } }

    QString dataHost() const { return _dataHost; }
    void setDataHost(const QString &host) { if (host != _dataHost) { _dataHost = host; emit dataHostChanged(); } }

    quint16 dataPort() const { return _dataPort; }
    void setDataPort(quint16 port) { if (port != _dataPort) { _dataPort = port; emit dataPortChanged(); } }

    QString authUrl() const { return _authUrl; }
    void setAuthUrl(const QString &url) { if (url != _authUrl) { _authUrl = url; emit authUrlChanged(); } }

    static QStringList supportedBaudRates();
    static QString cleanPortDisplayName(const QString &name);

signals:
    void baudChanged();
    void dataBitsChanged();
    void flowControlChanged();
    void stopBitsChanged();
    void parityChanged();
    void portNameChanged();
    void portDisplayNameChanged();
    void usbDirectChanged();
    void usernameChanged();
    void passwordChanged();
    void authHostChanged();
    void authPortChanged();
    void dataHostChanged();
    void dataPortChanged();
    void authUrlChanged();

private:
    qint32 _baud = QSerialPort::Baud57600;
    QSerialPort::DataBits _dataBits = QSerialPort::Data8;
    QSerialPort::FlowControl _flowControl = QSerialPort::NoFlowControl;
    QSerialPort::StopBits _stopBits = QSerialPort::OneStop;
    QSerialPort::Parity _parity = QSerialPort::NoParity;
    QString _portName;
    QString _portDisplayName;
    bool _usbDirect = false;
    QString _username;
    QString _password;
    QString _authHost = "127.0.0.1";
    quint16 _authPort = 8080;
    QString _dataHost = "127.0.0.1";
    quint16 _dataPort = 5760;
    QString _authUrl = ""; // Custom auth URL, if empty uses authHost:authPort
};

/*===========================================================================*/

class AuthenticatedSerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit AuthenticatedSerialWorker(const AuthenticatedSerialConfiguration *config, QObject *parent = nullptr);
    ~AuthenticatedSerialWorker();

    bool isConnected() const;
    bool isAuthenticated() const { return _authenticated; }

signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);
    void errorOccurred(const QString &errorString);
    void authenticationStatusChanged(bool authenticated);

public slots:
    void setupPorts();
    void connectToHosts();
    void disconnectFromHosts();
    void writeData(const QByteArray &data);

private slots:
    void _onDataSocketConnected();
    void _onDataSocketDisconnected();
    void _onDataSocketReadyRead();
    void _onDataSocketError(QAbstractSocket::SocketError error);
    void _onAuthSocketConnected();
    void _onAuthSocketDisconnected();
    void _onAuthSocketReadyRead();
    void _onAuthSocketError(QAbstractSocket::SocketError error);

private:
    void _authenticateUser();
    void _processAuthResponse(const QByteArray &data);
    void _sendSessionToken();

    const AuthenticatedSerialConfiguration *_serialConfig = nullptr;
    QTcpSocket *_dataSocket = nullptr;
    QTcpSocket *_authSocket = nullptr;
    QTimer *_timer = nullptr;
    bool _errorEmitted = false;
    bool _authenticated = false;
    QString _sessionToken;
};

/*===========================================================================*/

class AuthenticatedSerialLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit AuthenticatedSerialLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~AuthenticatedSerialLink();

    bool isConnected() const override;
    bool isSecureConnection() const override { return false; } // TCP is not considered secure without encryption

public slots:
    void disconnect() override;

private slots:
    void _onConnected();
    void _onDisconnected();
    void _onDataReceived(const QByteArray &data);
    void _onDataSent(const QByteArray &data);
    void _onErrorOccurred(const QString &errorString);
    void _onAuthenticationStatusChanged(bool authenticated);

private:
    bool _connect() override;
    void _writeBytes(const QByteArray &data) override;

    const AuthenticatedSerialConfiguration *_serialConfig = nullptr;
    AuthenticatedSerialWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;
};
