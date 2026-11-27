#pragma once

#include <QtCore/QObject>
#include <QtCore/QtPlugin>
#include <QtLocation/QGeoServiceProviderFactory>

/// Makes the mapping engine of the plugin, which has no geocoding, routing or places engine
class QGCGeoServiceProviderFactory : public QObject, public QGeoServiceProviderFactory
{
    Q_OBJECT
    Q_INTERFACES(QGeoServiceProviderFactory)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.geoservice.serviceproviderfactory/6.0" FILE "qgc_maps_plugin.json")

public:
    QGCGeoServiceProviderFactory(QObject* parent = nullptr);
    ~QGCGeoServiceProviderFactory();

    QGeoMappingManagerEngine* createMappingManagerEngine(const QVariantMap& parameters,
                                                         QGeoServiceProvider::Error* error,
                                                         QString* errorString) const final;
};
