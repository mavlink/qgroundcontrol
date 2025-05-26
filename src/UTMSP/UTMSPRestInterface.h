/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QString>
#include <QUrl>
#include <QPair>

class UTMSPRestInterface : public QObject {
    Q_OBJECT

public:
    UTMSPRestInterface(QObject *parent = nullptr);
    ~UTMSPRestInterface();

    enum class HostTarget {
        AuthClient,
        BlenderClient
    };

    void setBearerToken(const std::string &token);
    QPair<int, std::string> executeRequest();
    void modifyRequest(const QString &target, QNetworkAccessManager::Operation method, const QString &body = "");
    void setHost(const HostTarget &hostTarget);
    void setBasicToken(const QString &basicToken);

private:
    QNetworkAccessManager *             _networkManager = nullptr;
    QNetworkRequest                     _currentRequest;
    QString                             _currentBody;
    QNetworkAccessManager::Operation    _currentMethod;
    QString                             _currentURL;
    QString                             _basicToken;
};
