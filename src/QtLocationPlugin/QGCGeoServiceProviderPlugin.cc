#include "QGCGeoServiceProviderPlugin.h"

#include <QtCore/QThread>

#include "QGCGeoTiledMappingManagerEngine.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCGeoServiceProviderFactoryLog, "QtLocationPlugin.QGCGeoServiceProviderFactory")

QGCGeoServiceProviderFactory::QGCGeoServiceProviderFactory(QObject* parent)
    : QObject(parent)
{
    qCDebug(QGCGeoServiceProviderFactoryLog) << this;
}

QGCGeoServiceProviderFactory::~QGCGeoServiceProviderFactory()
{
    qCDebug(QGCGeoServiceProviderFactoryLog) << this;
}

QGeoMappingManagerEngine* QGCGeoServiceProviderFactory::createMappingManagerEngine(const QVariantMap& parameters,
                                                                                   QGeoServiceProvider::Error* error,
                                                                                   QString* errorString) const
{
    Q_UNUSED(parameters);

    // The mapping engine fetches its tiles with TileReply, which is for the main thread
    if (!QThread::isMainThread()) {
        qCCritical(QGCGeoServiceProviderFactoryLog) << "Internal Error: mapping engine asked for off the main thread";
        if (error) {
            *error = QGeoServiceProvider::LoaderError;
        }
        if (errorString) {
            *errorString = QStringLiteral("The mapping engine is for the main thread");
        }
        return nullptr;
    }

    return new QGCGeoTiledMappingManagerEngine(error, errorString);
}
