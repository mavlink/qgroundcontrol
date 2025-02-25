/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirLinkLink.h"
#include "AppSettings.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtNetwork/QUdpSocket>

QGC_LOGGING_CATEGORY(AirLinkLinkLog, "qgc.AirLink.AirLinklink");

AirLinkLink::AirLinkLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : UDPLink(config)
    , _AirLinkConfig(qobject_cast<const AirLinkConfiguration*>(config.get()))
{
    // qCDebug(AirLinkLinkLog) << Q_FUNC_INFO << this;

    (void) connect(this, &QThread::started, this, [this]() {
        QTimer *const pendingTimer = new QTimer();

        (void) connect(pendingTimer, &QTimer::timeout, this, [this, pendingTimer] {
            pendingTimer->setInterval(3000);
            if (_stillConnecting()) {
                qCDebug(AirLinkLinkLog) << "Connecting...";
                _sendLoginMsgToAirLink();
            } else {
                qCDebug(AirLinkLinkLog) << "Stopping...";
                pendingTimer->stop();
                pendingTimer->deleteLater();
            }
        }, Qt::QueuedConnection);

        pendingTimer->start(0);
    });

    _configureUdpSettings();
}

AirLinkLink::~AirLinkLink()
{
    // qCDebug(AirLinkLinkLog) << Q_FUNC_INFO << this;
}

void AirLinkLink::disconnect()
{
    _setConnectFlag(false);
    (void) UDPLink::disconnect();
}

bool AirLinkLink::_connect()
{
    std::shared_ptr<QMetaObject::Connection> conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived, this, [this, conn] (const LinkInterface* linkSrc, const mavlink_message_t &message) {
        if (this != linkSrc || message.msgid != MAVLINK_MSG_ID_AIRLINK_AUTH_RESPONSE) {
            return;
        }

        mavlink_airlink_auth_response_t responseMsg;
        mavlink_msg_airlink_auth_response_decode(&message, &responseMsg);
        const int answer = responseMsg.resp_type;
        if (answer != AIRLINK_AUTH_RESPONSE_TYPE::AIRLINK_AUTH_OK) {
            qCDebug(AirLinkLinkLog) << "AirLink auth failed";
            return;
        }

        qCDebug(AirLinkLinkLog) << "Connected successfully";
        (void) QObject::disconnect(*conn);
        _setConnectFlag(false);
    });

    _setConnectFlag(true);

    start(NormalPriority);

    return true;
}

void AirLinkLink::_configureUdpSettings()
{
    quint16 availablePort = 14550;
    QUdpSocket udpSocket;
    while (!udpSocket.bind(QHostAddress::LocalHost, availablePort)) {
        availablePort++;
    }

    UDPConfiguration *const udpConfig = dynamic_cast<UDPConfiguration*>(UDPLink::_config.get());
    udpConfig->addHost(_airLinkHost, _airLinkPort);
    udpConfig->setLocalPort(availablePort);
    udpConfig->setDynamic(false);
}

void AirLinkLink::_sendLoginMsgToAirLink()
{
    mavlink_airlink_auth_t auth{};

    const QString login = _AirLinkConfig->modemName(); ///< Connect not to account but to specific modem
    const QString pass = _AirLinkConfig->password();
    (void) strcpy(auth.login, login.toUtf8().constData());
    (void) strcpy(auth.password, pass.toUtf8().constData());

    mavlink_message_t mavmsg;
    (void) mavlink_msg_airlink_auth_pack(0, 0, &mavmsg, auth.login, auth.password);

    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buffer, &mavmsg);
    if (!_stillConnecting()) {
        qCDebug(AirLinkLinkLog) << "Force exit from connection";
        return;
    }

    writeBytesThreadSafe(reinterpret_cast<const char*>(buffer), len);
}

bool AirLinkLink::_stillConnecting()
{
    QMutexLocker locker(&_mutex);
    return _needToConnect;
}

void AirLinkLink::_setConnectFlag(bool connect)
{
    QMutexLocker locker(&_mutex);
    _needToConnect = connect;
}

/*===========================================================================*/

AirLinkConfiguration::AirLinkConfiguration(const QString &name, QObject *parent)
    : UDPConfiguration(name)
{
    // qCDebug(AirLinkLinkLog) << Q_FUNC_INFO << this;
}

AirLinkConfiguration::AirLinkConfiguration(const AirLinkConfiguration *copy, QObject *parent)
    : UDPConfiguration(copy)
{
    // qCDebug(AirLinkLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(copy);

    copyFrom(copy);
}

AirLinkConfiguration::~AirLinkConfiguration()
{
    // qCDebug(AirLinkLinkLog) << Q_FUNC_INFO << this;
}

void AirLinkConfiguration::setUsername(const QString &username)
{
    if (username != _username) {
        _username = username;
        emit usernameChanged();
    }
}

void AirLinkConfiguration::setPassword(const QString &password)
{
    if (password != _password) {
        _password = password;
        emit passwordChanged();
    }
}

void AirLinkConfiguration::setModemName(const QString &modemName)
{
    if (modemName != _modemName) {
        _modemName = modemName;
        emit modemNameChanged();
    }
}

void AirLinkConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    AppSettings *const appSettings = SettingsManager::instance()->appSettings();

    settings.beginGroup(root);

    setUsername(settings.value(_usernameSettingsKey, appSettings->loginAirLink()->rawValueString()).toString());
    setPassword(settings.value(_passwordSettingsKey, appSettings->passAirLink()->rawValueString()).toString());
    setModemName(settings.value(_modemNameSettingsKey).toString());

    settings.endGroup();
}

void AirLinkConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue(_usernameSettingsKey, _username);
    settings.setValue(_passwordSettingsKey, _password);
    settings.setValue(_modemNameSettingsKey, _modemName);

    settings.endGroup();
}

void AirLinkConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_CHECK_PTR(source);

    const AirLinkConfiguration *const AirLinkSource = qobject_cast<const AirLinkConfiguration*>(source);
    if (AirLinkSource) {
        setUsername(AirLinkSource->username());
        setPassword(AirLinkSource->password());
        setModemName(AirLinkSource->modemName());
    } else {
        qCWarning(AirLinkLinkLog) << "Internal error: cannot read AirLinkConfiguration from given source";
    }

    const UDPConfiguration *const udpSource = qobject_cast<const UDPConfiguration*>(source);
    if (udpSource) {
        UDPConfiguration::copyFrom(udpSource);
    }
}
