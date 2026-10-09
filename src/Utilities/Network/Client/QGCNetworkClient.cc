#include "QGCNetworkClient.h"

#include <QtCore/QCoreApplication>
#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkRequest>

namespace QGCNetworkHelper {
void setBasicAuth(QNetworkRequest& request, const QString& credentials)
{
    QHttpHeaders headers = request.headers();
    headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::Authorization, "Basic " + credentials);
    request.setHeaders(headers);
}

void setBasicAuth(QNetworkRequest& request, const QString& username, const QString& password)
{
    setBasicAuth(request, createBasicAuthCredentials(username, password));
}

void setBearerToken(QNetworkRequest& request, const QString& token)
{
    QHttpHeaders headers = request.headers();
    headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::Authorization, "Bearer " + token);
    request.setHeaders(headers);
}

QString createBasicAuthCredentials(const QString& username, const QString& password)
{
    const QString credentials = username + QLatin1Char(':') + password;
    return QString::fromLatin1(credentials.toUtf8().toBase64());
}

QString defaultUserAgent()
{
    static const QString userAgent = QStringLiteral("%1/%2 (Qt %3)")
                                         .arg(QCoreApplication::applicationName(),
                                              QCoreApplication::applicationVersion(), QString::fromLatin1(qVersion()));
    return userAgent;
}

QSslConfiguration createSslConfig()
{
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2OrLater);
    return config;
}

QNetworkAccessManager* createNetworkManager(QObject* parent)
{
    auto* manager = new QNetworkAccessManager(parent);
    configureProxy(manager);
    return manager;
}

void configureProxy(QNetworkAccessManager* manager)
{
    if (!manager) {
        return;
    }

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    QNetworkProxy proxy = manager->proxy();
    proxy.setType(QNetworkProxy::DefaultProxy);
    manager->setProxy(proxy);
#endif
}
}  // namespace QGCNetworkHelper
