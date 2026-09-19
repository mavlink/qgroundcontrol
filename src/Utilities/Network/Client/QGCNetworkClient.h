#pragma once

#include <QtCore/QString>

class QObject;
class QNetworkAccessManager;
class QNetworkRequest;

namespace QGCNetworkHelper {
/// Base64-encoded UTF-8 username:password, without the authentication scheme.
QString createBasicAuthCredentials(const QString& username, const QString& password);
void setBasicAuth(QNetworkRequest& request, const QString& credentials);
void setBasicAuth(QNetworkRequest& request, const QString& username, const QString& password);
void setBearerToken(QNetworkRequest& request, const QString& token);

QNetworkAccessManager* createNetworkManager(QObject* parent = nullptr);
void configureProxy(QNetworkAccessManager* manager);
}  // namespace QGCNetworkHelper
