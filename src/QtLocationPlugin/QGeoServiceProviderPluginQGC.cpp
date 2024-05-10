#include "QGeoServiceProviderPluginQGC.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGeoCodingManagerEngineQGC.h"
#include <QGCLoggingCategory.h>

#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

QGC_LOGGING_CATEGORY(QGeoServiceProviderFactoryQGCLog, "qgc.qtlocationplugin.qgeoserviceproviderfactoryqgc")

#ifndef CMAKE_LOCATION_PLUGIN
extern "C" Q_DECL_EXPORT QT_PREPEND_NAMESPACE(QPluginMetaData) qt_plugin_query_metadata_v2();
extern "C" Q_DECL_EXPORT QT_PREPEND_NAMESPACE(QObject) *qt_plugin_instance();

const QT_PREPEND_NAMESPACE(QStaticPlugin) qt_static_plugin_QGeoServiceProviderFactoryQGC()
{
   QT_PREPEND_NAMESPACE(QStaticPlugin) plugin = { qt_plugin_instance, qt_plugin_query_metadata_v2 };
   return plugin;
}
#endif

QGeoCodingManagerEngine* QGeoServiceProviderFactoryQGC::createGeocodingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    return new QGeoCodingManagerEngineQGC(parameters, error, errorString);
}

QGeoMappingManagerEngine* QGeoServiceProviderFactoryQGC::createMappingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    return new QGeoTiledMappingManagerEngineQGC(parameters, error, errorString);
}

QGeoRoutingManagerEngine* QGeoServiceProviderFactoryQGC::createRoutingManagerEngine(
    const QVariantMap &, QGeoServiceProvider::Error *, QString *) const
{
    // Not implemented for QGC
    return nullptr;
}

QPlaceManagerEngine* QGeoServiceProviderFactoryQGC::createPlaceManagerEngine(
    const QVariantMap &, QGeoServiceProvider::Error *, QString *) const
{
    // Not implemented for QGC
    return nullptr;
}

void QGeoServiceProviderFactoryQGC::setQmlEngine(QQmlEngine *engine)
{
    m_engine = engine;
    emit engineReady();

    qCDebug(QGeoServiceProviderFactoryQGCLog) << Q_FUNC_INFO << this;
}
