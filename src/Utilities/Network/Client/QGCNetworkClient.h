#pragma once

#include <QtCore/QString>
#include <QtNetwork/QSslConfiguration>

class QObject;
class QNetworkAccessManager;
class QNetworkRequest;

namespace QGCNetworkHelper {
/// Base64-encoded UTF-8 username:password, without the authentication scheme.
QString createBasicAuthCredentials(const QString& username, const QString& password);
/// @param credentials As createBasicAuthCredentials() returns them
void setBasicAuth(QNetworkRequest& request, const QString& credentials);
void setBasicAuth(QNetworkRequest& request, const QString& username, const QString& password);
/// @param token The token without the "Bearer " prefix
void setBearerToken(QNetworkRequest& request, const QString& token);

/// "<application>/<version> (Qt <version>)".
QString defaultUserAgent();

/// The default TLS configuration restricted to TLS 1.2 or later.
QSslConfiguration createSslConfig();

/// A manager that configureProxy() has set up. It belongs to @a parent, or to the caller when there is none.
QNetworkAccessManager* createNetworkManager(QObject* parent = nullptr);
/// Set up default proxy configuration on a network manager
void configureProxy(QNetworkAccessManager* manager);
}  // namespace QGCNetworkHelper
