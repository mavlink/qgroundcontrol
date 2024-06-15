/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPRestInterface.h"
#include "UTMSPLogger.h"

#include <QList>
#include <QNetworkInterface>
#include "qeventloop.h"

UTMSPRestInterface::UTMSPRestInterface(QObject *parent):
    QObject(parent)
{
    _networkManager = new QNetworkAccessManager(this);
}

UTMSPRestInterface::~UTMSPRestInterface()
{
    delete _networkManager;
    _networkManager = nullptr;
}


void UTMSPRestInterface::setHost(const QString &target)
{
    if (target == "AuthClient") {
        _currentURL = "https://passport.utm.dev.airoplatform.com";
        _currentRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    } else if (target == "BlenderClient") {
        _currentURL = "https://blender.utm.dev.airoplatform.com";
        _currentRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }
    _currentRequest.setRawHeader("User-Agent", QString("Qt/%1").arg(QT_VERSION_STR).toUtf8());
    _currentRequest.setRawHeader("Accept", "*/*");
    _currentRequest.setRawHeader("Accept-Encoding", "gzip, deflate, br");
    _currentRequest.setRawHeader("Connection", "keep-alive");

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1_3);
    _currentRequest.setSslConfiguration(sslConfig);
}

void UTMSPRestInterface::setBasicToken(const QString &basicToken){
    _basicToken = basicToken;
    _currentRequest.setRawHeader("Authorization", ("Basic " + _basicToken).toUtf8());
}

void UTMSPRestInterface::modifyRequest(const QString &target, QNetworkAccessManager::Operation method, const QString &body)
{
    QUrl url(_currentURL+target);
    _currentRequest.setUrl(url);
    _currentMethod = method;
    _currentBody = body;
}

QPair<int, QString> UTMSPRestInterface::executeRequest()
{
    if (!_networkManager) {
        qDebug() << "Network manager is not initialized!";
        return qMakePair(0, QString("Network manager is not initialized"));
    }

    QNetworkReply *reply = nullptr;
    switch(_currentMethod) {
    case QNetworkAccessManager::GetOperation:
        reply = _networkManager->get(_currentRequest);
        break;
    case QNetworkAccessManager::PostOperation:
        reply = _networkManager->post(_currentRequest, _currentBody.toUtf8());
        break;
    case QNetworkAccessManager::PutOperation:
        reply = _networkManager->put(_currentRequest, _currentBody.toUtf8());
        break;
    case QNetworkAccessManager::DeleteOperation:
        reply = _networkManager->deleteResource(_currentRequest);
        break;
    case QNetworkAccessManager::HeadOperation:
        reply = _networkManager->head(_currentRequest);
        break;
    default:
        qDebug() << "Unsupported HTTP method: " << _currentMethod;
        return qMakePair(0, QString("Unsupported HTTP method"));
    }

    if (!reply) return qMakePair(0, QString("Failed to create network reply"));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    auto response = reply->readAll();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    reply->deleteLater();

    return qMakePair(statusCode, QString::fromUtf8(response));
}

void UTMSPRestInterface::setBearerToken(const QString& token)
{
    _currentRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
}
