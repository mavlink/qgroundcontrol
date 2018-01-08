/*!
 *   @brief Desktop/Mobile RPC
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "QGCSyncFilesMobile.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AppSettings.h"

//-- TODO: This is here as it defines the UDP port and URL. It needs to go upstream
#include "TyphoonHQuickInterface.h"

QGC_LOGGING_CATEGORY(QGCSyncFiles, "QGCSyncFiles")

//-----------------------------------------------------------------------------
QGCSyncFilesMobile::QGCSyncFilesMobile(QObject* parent)
    : QGCRemoteSimpleSource(parent)
    , _udpSocket(NULL)
    , _remoteObject(NULL)
{
    connect(&_broadcastTimer,   &QTimer::timeout, this, &QGCSyncFilesMobile::_broadcastPresence);
    _broadcastTimer.setSingleShot(false);
    //-- Start UDP broadcast
    _broadcastTimer.start(5000);
}

//-----------------------------------------------------------------------------
QGCSyncFilesMobile::~QGCSyncFilesMobile()
{
    if(_udpSocket) {
        _udpSocket->deleteLater();
    }
    if(_remoteObject) {
        delete _remoteObject;
    }
}

//-----------------------------------------------------------------------------
//-- Slot for Desktop sendMission
void
QGCSyncFilesMobile::sendMission(QGCNewMission mission)
{
    QString missionFile = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
    if(!missionFile.endsWith("/")) missionFile += "/";
    missionFile += mission.name();
    if(!missionFile.endsWith(".plan")) missionFile += ".plan";
    qCDebug(QGCSyncFiles) << "Receiving:" << missionFile;
    QFile file(missionFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(mission.mission());
    }
}

//-----------------------------------------------------------------------------
void
QGCSyncFilesMobile::_broadcastPresence()
{
    //-- Mobile builds will broadcast their presence every 5 seconds so Desktop builds
    //   can find it.
    if(!_udpSocket) {
        _udpSocket = new QUdpSocket(this);
    }
    if(_macAddress.isEmpty()) {
        QUrl url;
        //-- Get first interface with a MAC address
        foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces()) {
            _macAddress = interface.hardwareAddress();
            if(!_macAddress.isEmpty() && !_macAddress.endsWith("00:00:00")) {
                //-- Get an URL to this host
                foreach (const QHostAddress &address, interface.allAddresses()) {
                    if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
                        if(!address.toString().startsWith("127")) {
                            url.setHost(address.toString());
                            url.setPort(QGC_RPC_PORT);
                            url.setScheme("tcp");
                            qCDebug(QGCSyncFiles) << "Remote Object URL:" << url.toString();
                            break;
                        }
                    }
                }
                break;
            }
        }
        if(_macAddress.length() > 9) {
            //-- Got one
            _macAddress = _macAddress.mid(9);
            _macAddress.replace(":", "");
        } else {
            //-- Make something up
            _macAddress.sprintf("%06d", (qrand() % 999999));
            qWarning() << "Could not get a proper MAC address. Using a random value.";
        }
        _macAddress = QGC_MOBILE_NAME + _macAddress;
        emit macAddressChanged();
        qCDebug(QGCSyncFiles) << "MAC Address:" << _macAddress;
        //-- Initialize Remote Object
        _remoteObject = new QRemoteObjectHost(url);
        _remoteObject->enableRemoting(this);
    }
    _udpSocket->writeDatagram(_macAddress.toLocal8Bit(), QHostAddress::Broadcast, QGC_UDP_BROADCAST_PORT);
}
