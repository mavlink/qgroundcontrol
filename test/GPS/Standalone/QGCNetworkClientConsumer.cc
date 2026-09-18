#include <QtCore/QCoreApplication>
#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include "QGCNetworkClient.h"

#if defined(QT_POSITIONING_LIB) || defined(QT_QML_LIB) || defined(QT_BLUETOOTH_LIB) || defined(QT_HTTPSERVER_LIB)
#error HTTP client helpers must not inherit positioning or server dependencies.
#endif

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    QNetworkRequest request;
    QGCNetworkHelper::setBasicAuth(request, QStringLiteral("User"), QStringLiteral("PaSs"));
    if (request.headers().value(QHttpHeaders::WellKnownHeader::Authorization) != "Basic VXNlcjpQYVNz") {
        return 1;
    }
    QGCNetworkHelper::setBearerToken(request, QStringLiteral("CaseSensitive"));
    if (request.headers().value(QHttpHeaders::WellKnownHeader::Authorization) != "Bearer CaseSensitive") {
        return 2;
    }
    return QGCNetworkHelper::createNetworkManager(&application) ? 0 : 3;
}
