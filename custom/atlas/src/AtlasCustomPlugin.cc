#include "AtlasCustomPlugin.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QFile>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/qqml.h>

#include "VideoManager.h"

Q_APPLICATION_STATIC(
    AtlasCustomPlugin,
    _atlasCustomPluginInstance
)

AtlasCustomPlugin::AtlasCustomPlugin(QObject *parent)
    : QGCCorePlugin(parent)
    , _jetsonClient(this)
    , _siyiClient(this)
    , _payloadRouter(&_jetsonClient, &_siyiClient, this)
    , _screenRecorder(this)
    , _kmlManager(this)
{
    // Instantiable 3D geometry used by the KML renderer. Register it before
    // any ATLAS FlyView QML is loaded.
    qmlRegisterType<AtlasKmlWallGeometry>("ATLAS.Kml", 1, 0, "AtlasKmlWallGeometry");
}

QGCCorePlugin *AtlasCustomPlugin::instance()
{
    return _atlasCustomPluginInstance();
}

QQmlApplicationEngine *AtlasCustomPlugin::createQmlApplicationEngine(QObject *parent)
{
    _qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);

    // Stable UI-facing API: QML controls atlasPayload; the router decides
    // whether the command goes directly to the A8 or through Jetson.
    _qmlEngine->rootContext()->setContextProperty(
        QStringLiteral("atlasPayload"),
        &_payloadRouter
    );

    // Concrete clients remain exposed only for diagnostics/development.
    _qmlEngine->rootContext()->setContextProperty(
        QStringLiteral("atlasJetsonClient"),
        &_jetsonClient
    );
    _qmlEngine->rootContext()->setContextProperty(
        QStringLiteral("atlasSiyiClient"),
        &_siyiClient
    );

    _qmlEngine->rootContext()->setContextProperty(
        QStringLiteral("atlasVideoManager"),
        VideoManager::instance()
    );

    // Local recording of the ATLAS application window. This is deliberately
    // independent from payload routing and from the VideoManager stream recorder.
    _qmlEngine->rootContext()->setContextProperty(
        QStringLiteral("atlasScreenRecorder"),
        &_screenRecorder
    );

    // Operator-loaded flight/restricted-area KML overlay. The same model is
    // projected by the native GeoMap in 2D and extruded in terrain 3D.
    _qmlEngine->rootContext()->setContextProperty(
        QStringLiteral("atlasKml"),
        &_kmlManager
    );

    _urlInterceptor = new AtlasOverrideInterceptor();
    _qmlEngine->addUrlInterceptor(_urlInterceptor);

    // Loads the operator-selected route from QSettings. No auto-detection and
    // no automatic failover are performed.
    _payloadRouter.start();

    return _qmlEngine;
}

void AtlasCustomPlugin::destroyQmlApplicationEngine(QQmlApplicationEngine *qmlEngine)
{
    if (qmlEngine && (qmlEngine == _qmlEngine) && _urlInterceptor) {
        qmlEngine->removeUrlInterceptor(_urlInterceptor);
        delete _urlInterceptor;
        _urlInterceptor = nullptr;
        _qmlEngine = nullptr;
    }

    QGCCorePlugin::destroyQmlApplicationEngine(qmlEngine);
}

AtlasOverrideInterceptor::AtlasOverrideInterceptor()
    : QQmlAbstractUrlInterceptor()
{
}

QUrl AtlasOverrideInterceptor::intercept(const QUrl &url,
                                         QQmlAbstractUrlInterceptor::DataType type)
{
    switch (type) {
    case QQmlAbstractUrlInterceptor::QmlFile:
    case QQmlAbstractUrlInterceptor::UrlString:
        if (url.scheme() == QStringLiteral("qrc")) {
            const QString originalPath = url.path();
            const QString overrideResource = QStringLiteral(":/Custom%1").arg(originalPath);

            if (QFile::exists(overrideResource)) {
                const QString relativePath = overrideResource.mid(2);
                QUrl result;
                result.setScheme(QStringLiteral("qrc"));
                result.setPath(QStringLiteral("/") + relativePath);
                return result;
            }
        }
        break;

    default:
        break;
    }

    return url;
}
