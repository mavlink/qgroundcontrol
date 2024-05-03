#include "AirlinkLink.h"
#include "AirLinkManager.h"
#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MAVLinkProtocol.h"

#include <QtNetwork/QUdpSocket>
#include <QtCore/QSettings>
#include <QtCore/QTimer>


AirlinkConfiguration::AirlinkConfiguration(const QString &name) : UDPConfiguration(name)
{

}

AirlinkConfiguration::AirlinkConfiguration(AirlinkConfiguration *source) : UDPConfiguration(source)
{
    _copyFrom(source);
}

AirlinkConfiguration::~AirlinkConfiguration()
{
}

void AirlinkConfiguration::setUsername(QString username)
{
    _username = username;
}

void AirlinkConfiguration::setPassword(QString password)
{
    _password = password;
}

void AirlinkConfiguration::setModemName(QString modemName)
{
    _modemName = modemName;
}

void AirlinkConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    AppSettings *appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    settings.beginGroup(root);
    _username = settings.value(_usernameSettingsKey, appSettings->loginAirLink()->rawValueString()).toString();
    _password = settings.value(_passwordSettingsKey, appSettings->passAirLink()->rawValueString()).toString();
    _modemName = settings.value(_modemNameSettingsKey).toString();
    settings.endGroup();
}

void AirlinkConfiguration::saveSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);
    settings.setValue(_usernameSettingsKey, _username);
    settings.setValue(_passwordSettingsKey, _password);
    settings.setValue(_modemNameSettingsKey, _modemName);
    settings.endGroup();
}

void AirlinkConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    auto* udpSource = qobject_cast<UDPConfiguration*>(source);
    if (udpSource) {
        UDPConfiguration::copyFrom(source);
    }
    _copyFrom(source);
}

void AirlinkConfiguration::_copyFrom(LinkConfiguration *source)
{
    auto* airlinkSource = qobject_cast<AirlinkConfiguration*>(source);
    if (airlinkSource) {
        _username = airlinkSource->username();
        _password = airlinkSource->password();
        _modemName = airlinkSource->modemName();
    } else {
        qWarning() << "Internal error: cannot read AirlinkConfiguration from given source";
    }
}


AirlinkLink::AirlinkLink(SharedLinkConfigurationPtr &config) : UDPLink(config)
{
    _configureUdpSettings();
}

AirlinkLink::~AirlinkLink()
{
}

// bool AirlinkLink::isConnected() const
// {
//     return UDPLink::isConnected();
// }

void AirlinkLink::disconnect()
{
    _setConnectFlag(false);
    UDPLink::disconnect();
}

// void AirlinkLink::run()
// {
//     UDPLink::run();
// }

bool AirlinkLink::_connect()
{
    start(NormalPriority);
    QTimer *pendingTimer = new QTimer;
    connect(pendingTimer, &QTimer::timeout, [this, pendingTimer] {
        pendingTimer->setInterval(3000);
        if (_stillConnecting()) {
            qDebug() << "Connecting...";
            _sendLoginMsgToAirLink();
        } else {
            qDebug() << "Stopping...";
            pendingTimer->stop();
            pendingTimer->deleteLater();
        }
    });
    MAVLinkProtocol *mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(mavlink, &MAVLinkProtocol::messageReceived, [this, conn] (LinkInterface* linkSrc, mavlink_message_t message) {
        if (this != linkSrc || message.msgid != MAVLINK_MSG_ID_AIRLINK_AUTH_RESPONSE) {
            return;
        }
        mavlink_airlink_auth_response_t responseMsg;
        mavlink_msg_airlink_auth_response_decode(&message, &responseMsg);
        int answer = responseMsg.resp_type;
        if (answer != AIRLINK_AUTH_RESPONSE_TYPE::AIRLINK_AUTH_OK) {
            qDebug() << "Airlink auth failed";
            return;
        }
        qDebug() << "Connected successfully";
        QObject::disconnect(*conn);
        _setConnectFlag(false);
    });
    _setConnectFlag(true);
    pendingTimer->start(0);
    return true;
}

void AirlinkLink::_configureUdpSettings()
{
    quint16 availablePort = 14550;
    QUdpSocket udpSocket;
    while (!udpSocket.bind(QHostAddress::LocalHost, availablePort))
        availablePort++;
    UDPConfiguration* udpConfig = dynamic_cast<UDPConfiguration*>(UDPLink::_config.get());
    udpConfig->addHost(AirLinkManager::airlinkHost, AirLinkManager::airlinkPort);
    udpConfig->setLocalPort(availablePort);
    udpConfig->setDynamic(false);
}

void AirlinkLink::_sendLoginMsgToAirLink()
{
    __mavlink_airlink_auth_t auth;
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    mavlink_message_t mavmsg;
    AirlinkConfiguration* config = dynamic_cast<AirlinkConfiguration*>(_config.get());
    QString login = config->modemName(); ///< Connect not to account but to specific modem
    QString pass  = config->password();

    memset(&auth.login, 0, sizeof(auth.login));
    memset(&auth.password, 0, sizeof(auth.password));
    strcpy(auth.login, login.toUtf8().constData());
    strcpy(auth.password, pass.toUtf8().constData());

    mavlink_msg_airlink_auth_pack(0, 0, &mavmsg, auth.login, auth.password);
    uint16_t len = mavlink_msg_to_send_buffer(buffer, &mavmsg);
    if (!_stillConnecting()) {
        qDebug() << "Force exit from connection";
        return;
    }
    writeBytesThreadSafe((const char *)buffer, len);
}

bool AirlinkLink::_stillConnecting()
{
    QMutexLocker locker(&_mutex);
    return _needToConnect;
}

void AirlinkLink::_setConnectFlag(bool connect)
{
    QMutexLocker locker(&_mutex);
    _needToConnect = connect;
}
