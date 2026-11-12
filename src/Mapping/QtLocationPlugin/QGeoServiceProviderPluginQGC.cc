#include "QGeoServiceProviderPluginQGC.h"

#include <QtCore/QThread>
#include <QtQml/QQmlEngine>

#include "QGCLoggingCategory.h"
#include "QGeoTiledMappingManagerEngineQGC.h"

QGC_LOGGING_CATEGORY(QGeoServiceProviderFactoryQGCLog, "QtLocationPlugin.QGeoServiceProviderFactoryQGC")

QGeoServiceProviderFactoryQGC::QGeoServiceProviderFactoryQGC(QObject* parent)
    : QObject(parent)
{
    qCDebug(QGeoServiceProviderFactoryQGCLog) << this;
}

QGeoServiceProviderFactoryQGC::~QGeoServiceProviderFactoryQGC()
{
    qCDebug(QGeoServiceProviderFactoryQGCLog) << this;
}

QGeoCodingManagerEngine* QGeoServiceProviderFactoryQGC::createGeocodingManagerEngine(const QVariantMap& parameters,
                                                                                     QGeoServiceProvider::Error* error,
                                                                                     QString* errorString) const
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

QGeoMappingManagerEngine* QGeoServiceProviderFactoryQGC::createMappingManagerEngine(const QVariantMap& parameters,
                                                                                    QGeoServiceProvider::Error* error,
                                                                                    QString* errorString) const
{
    Q_UNUSED(parameters);
    // The mapping engine fetches its tiles with QGCTileReply, which is for the main thread: the thread of the QML
    // engine that QtLocation sets before it asks for a mapping engine
    if (!m_engine || (m_engine->thread() != QThread::currentThread())) {
        qCWarning(QGeoServiceProviderFactoryQGCLog) << "No QML engine on this thread for the mapping manager engine";
        if (error) {
            *error = QGeoServiceProvider::LoaderError;
        }
        if (errorString) {
            *errorString = QStringLiteral("QML engine not available");
        }
        return nullptr;
    }

    if (error) {
        *error = QGeoServiceProvider::NoError;
    }
    if (errorString) {
        *errorString = "";
    }

    return new QGeoTiledMappingManagerEngineQGC(error, errorString);
}

QGeoRoutingManagerEngine* QGeoServiceProviderFactoryQGC::createRoutingManagerEngine(const QVariantMap& parameters,
                                                                                    QGeoServiceProvider::Error* error,
                                                                                    QString* errorString) const
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

QPlaceManagerEngine* QGeoServiceProviderFactoryQGC::createPlaceManagerEngine(const QVariantMap& parameters,
                                                                             QGeoServiceProvider::Error* error,
                                                                             QString* errorString) const
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
