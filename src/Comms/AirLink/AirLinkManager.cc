/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirLinkManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCLoggingCategory.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

QGC_LOGGING_CATEGORY(AirLinkManagerLog, "qgc.airlink.airlinkmanager");

Q_APPLICATION_STATIC(AirLinkManager, _airLinkManager);

AirLinkManager::AirLinkManager(QObject *parent)
    : QObject(parent)
    , _mngr(new QNetworkAccessManager(this))
{
    // qCDebug(AirLinkManagerLog) << Q_FUNC_INFO << this;
}

AirLinkManager::~AirLinkManager()
{
    // qCDebug(AirLinkManagerLog) << Q_FUNC_INFO << this;
}

AirLinkManager *AirLinkManager::instance()
{
    return _airLinkManager();
}

bool AirLinkManager::isOnline(const QString &drone)
{
    return _vehiclesFromServer.contains(drone) ? _vehiclesFromServer[drone] : false;
}

void AirLinkManager::updateCredentials(const QString &login, const QString &pass)
{
    SettingsManager::instance()->appSettings()->loginAirLink()->setRawValue(login);
    SettingsManager::instance()->appSettings()->passAirLink()->setRawValue(pass);
}

void AirLinkManager::_connectToAirLinkServer(const QString &login, const QString &pass)
{
    const QUrl url("https://air-link.space/api/gs/getModems");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["login"] = login;
    obj["password"] = pass;
    QJsonDocument doc(obj);
    const QByteArray data = doc.toJson();

    QNetworkReply *const reply = _mngr->post(request, data);
    (void) QObject::connect(reply, &QNetworkReply::finished, this, &AirLinkManager::_processReplyAirlinkServer);
}

void AirLinkManager::_processReplyAirlinkServer()
{
    QNetworkReply* const reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qCDebug(AirLinkManagerLog) << "Airlink auth - network error";
        return;
    }

    const QByteArray ba = reply->readAll();

    if (!QJsonDocument::fromJson(ba)["modems"].toArray().isEmpty()) {
        _parseAnswer(ba);
    } else {
        qCDebug(AirLinkManagerLog) << "No airlink modems in answer";
    }
}

void AirLinkManager::_parseAnswer(const QByteArray &ba)
{
    _vehiclesFromServer.clear();

    for (const auto &arr : QJsonDocument::fromJson(ba)["modems"].toArray()) {
        const QString droneModem = arr.toObject()["name"].toString();
        const bool isOnline = arr.toObject()["isOnline"].toBool();
        _vehiclesFromServer[droneModem] = isOnline;
    }

    emit droneListChanged();
}
