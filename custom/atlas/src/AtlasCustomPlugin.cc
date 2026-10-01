#include "AtlasCustomPlugin.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QFile>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>

#include "VideoManager.h"

Q_APPLICATION_STATIC(
    AtlasCustomPlugin,
    _atlasCustomPluginInstance
)

AtlasCustomPlugin::AtlasCustomPlugin(QObject *parent)
    : QGCCorePlugin(parent)
    , _jetsonClient(this)
{
}

QGCCorePlugin *AtlasCustomPlugin::instance()
{
    return _atlasCustomPluginInstance();
}

QQmlApplicationEngine *AtlasCustomPlugin::createQmlApplicationEngine(
    QObject *parent
)
{
    _qmlEngine =
        QGCCorePlugin::createQmlApplicationEngine(parent);

    // Disponibiliza o cliente diretamente em QML:
    //
    // atlasJetsonClient.gimbalRate(...)
    // atlasJetsonClient.gimbalStop()
    // atlasJetsonClient.zoom(...)
    // etc.
    _qmlEngine
        ->rootContext()
        ->setContextProperty(
            QStringLiteral("atlasJetsonClient"),
            &_jetsonClient
        );

    _qmlEngine
        ->rootContext()
        ->setContextProperty(
            QStringLiteral("atlasVideoManager"),
            VideoManager::instance()
        );

    // Permite substituir ficheiros QML originais por versões
    // presentes em /Custom/qml/...
    _urlInterceptor =
        new AtlasOverrideInterceptor();

    _qmlEngine->addUrlInterceptor(
        _urlInterceptor
    );

    // Por enquanto ligamos ao Mock Jetson local.
    //
    // Mais tarde:
    // 127.0.0.1 -> IP real da Jetson através do HM30.
    _jetsonClient.connectToHost(
        QStringLiteral("127.0.0.1"),
        45900
    );

    return _qmlEngine;
}

void AtlasCustomPlugin::destroyQmlApplicationEngine(
    QQmlApplicationEngine *qmlEngine
)
{
    if (qmlEngine &&
        (qmlEngine == _qmlEngine) &&
        _urlInterceptor) {

        qmlEngine->removeUrlInterceptor(
            _urlInterceptor
        );

        delete _urlInterceptor;

        _urlInterceptor = nullptr;
        _qmlEngine = nullptr;
    }

    QGCCorePlugin::destroyQmlApplicationEngine(
        qmlEngine
    );
}


// -------------------------------------------------------------------------
// QML OVERRIDE
//
// Exemplo:
//
// qrc:/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml
//
// passa para:
//
// qrc:/Custom/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml
//
// quando existir um override.
// -------------------------------------------------------------------------

AtlasOverrideInterceptor::AtlasOverrideInterceptor()
    : QQmlAbstractUrlInterceptor()
{
}

QUrl AtlasOverrideInterceptor::intercept(
    const QUrl &url,
    QQmlAbstractUrlInterceptor::DataType type
)
{
    switch (type) {

    case QQmlAbstractUrlInterceptor::QmlFile:
    case QQmlAbstractUrlInterceptor::UrlString:

        if (url.scheme() == QStringLiteral("qrc")) {

            const QString originalPath =
                url.path();

            const QString overrideResource =
                QStringLiteral(":/Custom%1")
                    .arg(originalPath);

            if (QFile::exists(overrideResource)) {

                const QString relativePath =
                    overrideResource.mid(2);

                QUrl result;

                result.setScheme(
                    QStringLiteral("qrc")
                );

                result.setPath(
                    QStringLiteral("/") +
                    relativePath
                );

                return result;
            }
        }

        break;

    default:
        break;
    }

    return url;
}
