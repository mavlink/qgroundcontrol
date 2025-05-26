/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QtPlugin>
#include <QtCore/QLoggingCategory>
#include <QtLocation/QGeoServiceProviderFactory>

Q_DECLARE_LOGGING_CATEGORY(QGeoServiceProviderFactoryQGCLog)

class QGeoServiceProviderFactoryQGC: public QObject, public QGeoServiceProviderFactory
{
    Q_OBJECT
    Q_INTERFACES(QGeoServiceProviderFactory)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.geoservice.serviceproviderfactory/6.0" FILE "qgc_maps_plugin.json")

public:
    QGeoServiceProviderFactoryQGC(QObject *parent = nullptr);
    ~QGeoServiceProviderFactoryQGC();

    QGeoCodingManagerEngine* createGeocodingManagerEngine(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const final;
    QGeoMappingManagerEngine* createMappingManagerEngine(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const final;
    QGeoRoutingManagerEngine* createRoutingManagerEngine(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const final;
    QPlaceManagerEngine* createPlaceManagerEngine(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const final;

    void setQmlEngine(QQmlEngine* engine) final { m_engine = engine; }

private:
    QQmlEngine *m_engine = nullptr;
};
