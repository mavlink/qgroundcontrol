/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGeoServiceProviderPluginQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include <QGCLoggingCategory.h>

#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(QGeoServiceProviderFactoryQGCLog, "qgc.qtlocationplugin.qgeoserviceproviderfactoryqgc")

QGeoServiceProviderFactoryQGC::QGeoServiceProviderFactoryQGC(QObject *parent)
    : QObject(parent)
{
    qCDebug(QGeoServiceProviderFactoryQGCLog) << Q_FUNC_INFO << this;
}

QGeoServiceProviderFactoryQGC::~QGeoServiceProviderFactoryQGC()
{
    qCDebug(QGeoServiceProviderFactoryQGCLog) << Q_FUNC_INFO << this;
}

QGeoCodingManagerEngine *QGeoServiceProviderFactoryQGC::createGeocodingManagerEngine(
   const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(parameters);
    if (error) {
        *error = QGeoServiceProvider::NotSupportedError;
    }
    if (errorString) {
        *errorString = "Geocoding Not Supported";
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

    QNetworkAccessManager *networkManager = nullptr;
    if (m_engine) {
        networkManager = m_engine->networkAccessManager();
    }

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
        *errorString = "Routing Not Supported";
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
        *errorString = "Place Not Supported";
    }

    return nullptr;
}
