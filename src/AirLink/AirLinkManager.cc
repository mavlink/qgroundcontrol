/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirLinkManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "LinkManager.h"
#include "SettingsManager.h"

#include <QSettings>
#include <QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>

const QString AirLinkManager::airlinkHost = "air-link.space";

AirLinkManager::AirLinkManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

AirLinkManager::~AirLinkManager()
{
}

void AirLinkManager::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
}

QStringList AirLinkManager::droneList() const
{
    return _vehiclesFromServer.keys();
}

void AirLinkManager::updateDroneList(const QString &login, const QString &pass)
{
    connectToAirLinkServer(login, pass);
}

bool AirLinkManager::isOnline(const QString &drone)
{
    if (!_vehiclesFromServer.contains(drone)) {
        return false;
    } else {
        return _vehiclesFromServer[drone];
    }
}

void AirLinkManager::connectToAirLinkServer(const QString &login, const QString &pass)
{
    QNetworkAccessManager *mngr = new QNetworkAccessManager(this);

    const QUrl url("https://air-link.space/api/gs/getModems");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["login"] = login;
    obj["password"] = pass;
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();

    _reply = mngr->post(request, data);

    QObject::connect(_reply, &QNetworkReply::finished, [this](){
        _processReplyAirlinkServer(*_reply);
        _reply->deleteLater();
    });

    mngr = nullptr;
    delete mngr;
}

void AirLinkManager::updateCredentials(const QString &login, const QString &pass)
{
    _toolbox->settingsManager()->appSettings()->loginAirLink()->setRawValue(login);
    _toolbox->settingsManager()->appSettings()->passAirLink()->setRawValue(pass);
}

void AirLinkManager::_parseAnswer(const QByteArray &ba)
{
    _vehiclesFromServer.clear();
    for (const auto &arr : QJsonDocument::fromJson(ba)["modems"].toArray()) {
        QString droneModem = arr.toObject()["name"].toString();
        bool isOnline = arr.toObject()["isOnline"].toBool();
        _vehiclesFromServer[droneModem] = isOnline;
    }
    emit droneListChanged();
}

void AirLinkManager::_processReplyAirlinkServer(QNetworkReply &reply)
{
    QByteArray ba = reply.readAll();

    if (reply.error() == QNetworkReply::NoError) {
        if (!QJsonDocument::fromJson(ba)["modems"].toArray().isEmpty()) {
            _parseAnswer(ba);
        } else {
            qDebug() << "No airlink modems in answer";
        }
    } else {
        qDebug() << "Airlink auth - network error";
    }
}

