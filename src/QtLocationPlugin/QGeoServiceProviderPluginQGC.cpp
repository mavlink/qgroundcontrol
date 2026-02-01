#include "QGeoServiceProviderPluginQGC.h"

#include <QtCore/QThread>
#include <QtQml/QQmlEngine>

#include <QtCore/QLoggingCategory>
#include "QGeoTiledMappingManagerEngineQGC.h"

Q_STATIC_LOGGING_CATEGORY(QGeoServiceProviderFactoryQGCLog, "QtLocationPlugin.QGeoServiceProviderFactoryQGC")

QGeoServiceProviderFactoryQGC::QGeoServiceProviderFactoryQGC(QObject *parent)
    : QObject(parent)
{
    qCDebug(QGeoServiceProviderFactoryQGCLog) << this;
}

QGeoServiceProviderFactoryQGC::~QGeoServiceProviderFactoryQGC()
{
    qCDebug(QGeoServiceProviderFactoryQGCLog) << this;
}

QGeoCodingManagerEngine *QGeoServiceProviderFactoryQGC::createGeocodingManagerEngine(
   const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(parameters);
    if (error) {
        *error = QGeoServiceProvider::NotSupportedError;
    }
    if (errorString) {
        *errorString = QStringLiteral("Geocoding Not Supported");
    }

    return nullptr;
}

QGeoMappingManagerEngine *QGeoServiceProviderFactoryQGC::createMappingManagerEngine(
   const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    if (error) {
        *error = QGeoServiceProvider::NoError;
    }
    if (errorString) {
        *errorString = "";
    }

    Q_ASSERT(m_engine);
    Q_ASSERT(m_engine->thread() == QThread::currentThread());

    QNetworkAccessManager *networkManager = m_engine->networkAccessManager();

    return new QGeoTiledMappingManagerEngineQGC(parameters, error, errorString, networkManager, nullptr);
}

QGeoRoutingManagerEngine *QGeoServiceProviderFactoryQGC::createRoutingManagerEngine(
   const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(parameters);
    if (error) {
        *error = QGeoServiceProvider::NotSupportedError;
    }
    if (errorString) {
        *errorString = QStringLiteral("Routing Not Supported");
    }

    return nullptr;
}

QPlaceManagerEngine *QGeoServiceProviderFactoryQGC::createPlaceManagerEngine(
   const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(parameters);
    if (error) {
        *error = QGeoServiceProvider::NotSupportedError;
    }
    if (errorString) {
        *errorString = QStringLiteral("Place Not Supported");
    }

    return nullptr;
}
