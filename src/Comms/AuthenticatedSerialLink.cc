/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AuthenticatedSerialLink.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QThread>
#include <QtNetwork/QTcpSocket>

Q_LOGGING_CATEGORY(AuthenticatedSerialLinkLog, "qgc.comms.authenticatedserialllink")

namespace {
    constexpr int CONNECT_TIMEOUT_MS = 1000;
    constexpr int DISCONNECT_TIMEOUT_MS = 3000;
    constexpr int READ_TIMEOUT_MS = 100;
    constexpr int AUTH_TIMEOUT_MS = 5000;
}

/*===========================================================================*/

AuthenticatedSerialConfiguration::AuthenticatedSerialConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    qCDebug(AuthenticatedSerialLinkLog) << this;
}

AuthenticatedSerialConfiguration::AuthenticatedSerialConfiguration(const AuthenticatedSerialConfiguration *source, QObject *parent)
    : LinkConfiguration(source, parent)
{
    qCDebug(AuthenticatedSerialLinkLog) << this;
    AuthenticatedSerialConfiguration::copyFrom(source);
}

AuthenticatedSerialConfiguration::~AuthenticatedSerialConfiguration()
{
    qCDebug(AuthenticatedSerialLinkLog) << this;
}

void AuthenticatedSerialConfiguration::setPortName(const QString &name)
{
    const QString portName = name.trimmed();
    if (portName.isEmpty()) {
        return;
    }

    if (portName != _portName) {
        _portName = portName;
        emit portNameChanged();
    }

    const QString portDisplayName = cleanPortDisplayName(portName);
    setPortDisplayName(portDisplayName);
}

void AuthenticatedSerialConfiguration::copyFrom(const LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);

    const AuthenticatedSerialConfiguration* const usource = qobject_cast<const AuthenticatedSerialConfiguration*>(source);
    if (usource) {
        setPortName(usource->portName());
        setUsername(usource->username());
        setPassword(usource->password());
        setAuthHost(usource->authHost());
        setAuthPort(usource->authPort());
        setDataHost(usource->dataHost());
        setDataPort(usource->dataPort());
        setAuthUrl(usource->authUrl());
    }
}

void AuthenticatedSerialConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setPortName(settings.value(QStringLiteral("portName"), _portName).toString());
    setUsername(settings.value(QStringLiteral("username"), _username).toString());
    setPassword(settings.value(QStringLiteral("password"), _password).toString());
    setAuthHost(settings.value(QStringLiteral("authHost"), _authHost).toString());
    setAuthPort(static_cast<quint16>(settings.value(QStringLiteral("authPort"), _authPort).toUInt()));
    setDataHost(settings.value(QStringLiteral("dataHost"), _dataHost).toString());
    setDataPort(static_cast<quint16>(settings.value(QStringLiteral("dataPort"), _dataPort).toUInt()));
    setAuthUrl(settings.value(QStringLiteral("authUrl"), _authUrl).toString());

    settings.endGroup();
}

void AuthenticatedSerialConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue(QStringLiteral("portName"), _portName);
    settings.setValue(QStringLiteral("username"), _username);
    settings.setValue(QStringLiteral("password"), _password);
    settings.setValue(QStringLiteral("authHost"), _authHost);
    settings.setValue(QStringLiteral("authPort"), _authPort);
    settings.setValue(QStringLiteral("dataHost"), _dataHost);
    settings.setValue(QStringLiteral("dataPort"), _dataPort);
    settings.setValue(QStringLiteral("authUrl"), _authUrl);

    settings.endGroup();
}

QStringList AuthenticatedSerialConfiguration::supportedBaudRates()
{
    return QStringList() << QStringLiteral("1200")
                        << QStringLiteral("2400")
                        << QStringLiteral("4800")
                        << QStringLiteral("9600")
                        << QStringLiteral("19200")
                        << QStringLiteral("38400")
                        << QStringLiteral("57600")
                        << QStringLiteral("115200")
                        << QStringLiteral("230400")
                        << QStringLiteral("460800")
                        << QStringLiteral("921600");
}

QString AuthenticatedSerialConfiguration::cleanPortDisplayName(const QString &name)
{
    QString displayName = name.trimmed();
    #ifdef Q_OS_WIN
    static const QRegularExpression winComRegex{R"(\s*\(COM\d*\))"};
    displayName = displayName.remove(winComRegex);
    #endif
    return displayName;
}

/*===========================================================================*/

AuthenticatedSerialWorker::AuthenticatedSerialWorker(const AuthenticatedSerialConfiguration *config, QObject *parent)
    : QObject(parent)
    , _serialConfig(config)
{
    qCDebug(AuthenticatedSerialLinkLog) << this;
}

AuthenticatedSerialWorker::~AuthenticatedSerialWorker()
{
    if (_dataSocket) {
        _dataSocket->deleteLater();
    }

    if (_authSocket) {
        _authSocket->deleteLater();
    }

    delete _timer;
    _timer = nullptr;

    qCDebug(AuthenticatedSerialLinkLog) << this;
}

bool AuthenticatedSerialWorker::isConnected() const
{
    return _dataSocket && _dataSocket->state() == QAbstractSocket::ConnectedState && _authenticated;
}

void AuthenticatedSerialWorker::setupPorts()
{
    Q_ASSERT(!_dataSocket);
    _dataSocket = new QTcpSocket(this);

    Q_ASSERT(!_authSocket);
    _authSocket = new QTcpSocket(this);

    Q_ASSERT(!_timer);
    _timer = new QTimer(this);

    // Connect data socket signals
    (void) connect(_dataSocket, &QTcpSocket::connected, this, &AuthenticatedSerialWorker::_onDataSocketConnected);
    (void) connect(_dataSocket, &QTcpSocket::disconnected, this, &AuthenticatedSerialWorker::_onDataSocketDisconnected);
    (void) connect(_dataSocket, &QTcpSocket::readyRead, this, &AuthenticatedSerialWorker::_onDataSocketReadyRead);
    (void) connect(_dataSocket, &QTcpSocket::errorOccurred, this, &AuthenticatedSerialWorker::_onDataSocketError);

    // Connect auth socket signals
    (void) connect(_authSocket, &QTcpSocket::connected, this, &AuthenticatedSerialWorker::_onAuthSocketConnected);
    (void) connect(_authSocket, &QTcpSocket::disconnected, this, &AuthenticatedSerialWorker::_onAuthSocketDisconnected);
    (void) connect(_authSocket, &QTcpSocket::readyRead, this, &AuthenticatedSerialWorker::_onAuthSocketReadyRead);
    (void) connect(_authSocket, &QTcpSocket::errorOccurred, this, &AuthenticatedSerialWorker::_onAuthSocketError);

    _timer->setSingleShot(false);
    _timer->start(1000); // Check connection status every second
}

void AuthenticatedSerialWorker::connectToHosts()
{
    if (isConnected()) {
        qCWarning(AuthenticatedSerialLinkLog) << "Already connected";
        return;
    }

    // Start authentication process first
    qCDebug(AuthenticatedSerialLinkLog) << "Starting authentication process";
    _authenticateUser();
}

void AuthenticatedSerialWorker::disconnectFromHosts()
{
    if (!isConnected()) {
        qCDebug(AuthenticatedSerialLinkLog) << "Already disconnected";
        return;
    }

    qCDebug(AuthenticatedSerialLinkLog) << "Disconnecting from hosts";
    
    _authenticated = false;
    _sessionToken.clear();
    emit authenticationStatusChanged(_authenticated);

    if (_authSocket->state() == QAbstractSocket::ConnectedState) {
        _authSocket->disconnectFromHost();
    }

    if (_dataSocket->state() == QAbstractSocket::ConnectedState) {
        _dataSocket->disconnectFromHost();
    }

    emit disconnected();
}

void AuthenticatedSerialWorker::writeData(const QByteArray &data)
{
    if (!isConnected() || !_authenticated) {
        qCWarning(AuthenticatedSerialLinkLog) << "Attempting to write to disconnected or unauthenticated connection";
        return;
    }

    const qint64 bytesWritten = _dataSocket->write(data);
    if (bytesWritten == -1) {
        emit errorOccurred(tr("Failed to write data: %1").arg(_dataSocket->errorString()));
    } else if (bytesWritten != data.size()) {
        qCWarning(AuthenticatedSerialLinkLog) << "Failed to write all data. Requested:" << data.size() << "Written:" << bytesWritten;
    }

    emit dataSent(data.left(static_cast<int>(bytesWritten)));
}

void AuthenticatedSerialWorker::_authenticateUser()
{
    if (_authSocket->state() == QAbstractSocket::ConnectedState) {
        _authSocket->disconnectFromHost();
        _authSocket->waitForDisconnected(1000);
    }

    QString host = _serialConfig->authHost();
    quint16 port = _serialConfig->authPort();
    
    // Use custom URL if provided, otherwise use host:port
    if (!_serialConfig->authUrl().isEmpty()) {
        // Parse custom URL (format: host:port or full URL)
        QString customUrl = _serialConfig->authUrl().trimmed();
        if (customUrl.contains("://")) {
            // Full URL like http://example.com:8080/auth
            qCDebug(AuthenticatedSerialLinkLog) << "Using custom auth URL:" << customUrl;
            // For now, extract host and port from URL
            QStringList parts = customUrl.split("://");
            if (parts.length() > 1) {
                QStringList hostParts = parts[1].split(":");
                if (hostParts.length() > 1) {
                    host = hostParts[0];
                    port = hostParts[1].split("/")[0].toInt();
                }
            }
        } else if (customUrl.contains(":")) {
            // Simple host:port format
            QStringList parts = customUrl.split(":");
            if (parts.length() == 2) {
                host = parts[0];
                port = parts[1].toInt();
            }
        } else {
            // Just hostname, use default port
            host = customUrl;
        }
    }

    qCDebug(AuthenticatedSerialLinkLog) << "Connecting to authentication server at" << host << ":" << port;
    _authSocket->connectToHost(host, port);

    if (!_authSocket->waitForConnected(AUTH_TIMEOUT_MS)) {
        emit errorOccurred(tr("Failed to connect to authentication server: %1").arg(_authSocket->errorString()));
        return;
    }
}

void AuthenticatedSerialWorker::_onAuthSocketConnected()
{
    qCDebug(AuthenticatedSerialLinkLog) << "Connected to authentication server, sending credentials";

    // Create authentication request JSON
    QJsonObject authRequest;
    authRequest["username"] = _serialConfig->username();
    authRequest["password"] = _serialConfig->password();
    authRequest["action"] = "login";

    QJsonDocument doc(authRequest);
    QByteArray requestData = doc.toJson(QJsonDocument::Compact) + "\n";
    
    _authSocket->write(requestData);
}

void AuthenticatedSerialWorker::_onAuthSocketDisconnected()
{
    qCDebug(AuthenticatedSerialLinkLog) << "Disconnected from authentication server";
}

void AuthenticatedSerialWorker::_onAuthSocketReadyRead()
{
    QByteArray data = _authSocket->readAll();
    _processAuthResponse(data);
}

void AuthenticatedSerialWorker::_onAuthSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(tr("Authentication socket error: %1").arg(_authSocket->errorString()));
}

void AuthenticatedSerialWorker::_processAuthResponse(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred(tr("Failed to parse authentication response: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject response = doc.object();
    QString status = response["status"].toString();
    
    if (status == "success") {
        _authenticated = true;
        _sessionToken = response["session_token"].toString();
        
        emit authenticationStatusChanged(_authenticated);
        qCDebug(AuthenticatedSerialLinkLog) << "Authentication successful, connecting to data port";
        
        // Close auth socket and connect to data port
        _authSocket->disconnectFromHost();
        
        // Connect to data port
        QString dataHost = _serialConfig->dataHost();
        quint16 dataPort = _serialConfig->dataPort();
        
        qCDebug(AuthenticatedSerialLinkLog) << "Connecting to data server at" << dataHost << ":" << dataPort;
        _dataSocket->connectToHost(dataHost, dataPort);
        
        if (!_dataSocket->waitForConnected(AUTH_TIMEOUT_MS)) {
            emit errorOccurred(tr("Failed to connect to data server: %1").arg(_dataSocket->errorString()));
            _authenticated = false;
            emit authenticationStatusChanged(_authenticated);
            return;
        }
        
    } else {
        QString message = response["message"].toString();
        emit errorOccurred(tr("Authentication failed: %1").arg(message.isEmpty() ? "Unknown error" : message));
        _authenticated = false;
        emit authenticationStatusChanged(_authenticated);
    }
}

void AuthenticatedSerialWorker::_sendSessionToken()
{
    if (_dataSocket->state() != QAbstractSocket::ConnectedState || _sessionToken.isEmpty()) {
        return;
    }

    // Send session token as first message to data server
    QJsonObject tokenMessage;
    tokenMessage["action"] = "authenticate";
    tokenMessage["session_token"] = _sessionToken;
    
    QJsonDocument doc(tokenMessage);
    QByteArray tokenData = doc.toJson(QJsonDocument::Compact) + "\n";
    
    _dataSocket->write(tokenData);
    qCDebug(AuthenticatedSerialLinkLog) << "Sent session token to data server";
}

void AuthenticatedSerialWorker::_onDataSocketConnected()
{
    qCDebug(AuthenticatedSerialLinkLog) << "Connected to data server";
    _sendSessionToken();
    emit connected();
}

void AuthenticatedSerialWorker::_onDataSocketDisconnected()
{
    qCDebug(AuthenticatedSerialLinkLog) << "Disconnected from data server";
    _authenticated = false;
    emit authenticationStatusChanged(_authenticated);
    emit disconnected();
}

void AuthenticatedSerialWorker::_onDataSocketReadyRead()
{
    QByteArray data = _dataSocket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void AuthenticatedSerialWorker::_onDataSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(tr("Data socket error: %1").arg(_dataSocket->errorString()));
}

/*===========================================================================*/

AuthenticatedSerialLink::AuthenticatedSerialLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _serialConfig(qobject_cast<const AuthenticatedSerialConfiguration*>(config.get()))
    , _worker(new AuthenticatedSerialWorker(_serialConfig))
    , _workerThread(new QThread(this))
{
    qCDebug(AuthenticatedSerialLinkLog) << this;

    _workerThread->setObjectName(QStringLiteral("AuthenticatedSerial_%1").arg(_serialConfig->name()));

    (void) _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &AuthenticatedSerialWorker::setupPorts);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &AuthenticatedSerialWorker::connected, this, &AuthenticatedSerialLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &AuthenticatedSerialWorker::disconnected, this, &AuthenticatedSerialLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &AuthenticatedSerialWorker::dataReceived, this, &AuthenticatedSerialLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker, &AuthenticatedSerialWorker::dataSent, this, &AuthenticatedSerialLink::_onDataSent, Qt::QueuedConnection);
    (void) connect(_worker, &AuthenticatedSerialWorker::errorOccurred, this, &AuthenticatedSerialLink::_onErrorOccurred, Qt::QueuedConnection);
    (void) connect(_worker, &AuthenticatedSerialWorker::authenticationStatusChanged, this, &AuthenticatedSerialLink::_onAuthenticationStatusChanged, Qt::QueuedConnection);

    _workerThread->start();
}

AuthenticatedSerialLink::~AuthenticatedSerialLink()
{
    (void) QMetaObject::invokeMethod(_worker, "disconnectFromHosts", Qt::BlockingQueuedConnection);

    _workerThread->quit();
    if (!_workerThread->wait(DISCONNECT_TIMEOUT_MS)) {
        qCWarning(AuthenticatedSerialLinkLog) << "Failed to wait for Authenticated Serial Thread to close";
    }

    qCDebug(AuthenticatedSerialLinkLog) << this;
}

bool AuthenticatedSerialLink::isConnected() const
{
    return _worker->isConnected() && _worker->isAuthenticated();
}

bool AuthenticatedSerialLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectToHosts", Qt::QueuedConnection);
}

void AuthenticatedSerialLink::disconnect()
{
    (void) QMetaObject::invokeMethod(_worker, "disconnectFromHosts", Qt::QueuedConnection);
}

void AuthenticatedSerialLink::_onConnected()
{
    emit connected();
}

void AuthenticatedSerialLink::_onDisconnected()
{
    emit disconnected();
}

void AuthenticatedSerialLink::_onDataReceived(const QByteArray &data)
{
    emit bytesReceived(this, data);
}

void AuthenticatedSerialLink::_onDataSent(const QByteArray &data)
{
    emit bytesSent(this, data);
}

void AuthenticatedSerialLink::_onErrorOccurred(const QString &errorString)
{
    emit communicationError(tr("Authenticated TCP Link Error"), errorString);
}

void AuthenticatedSerialLink::_onAuthenticationStatusChanged(bool authenticated)
{
    qCDebug(AuthenticatedSerialLinkLog) << "Authentication status changed:" << authenticated;
    // Could emit additional signals here if needed for UI updates
}

void AuthenticatedSerialLink::_writeBytes(const QByteArray &data)
{
    (void) QMetaObject::invokeMethod(_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, data));
}
