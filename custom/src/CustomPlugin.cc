#include "CustomPlugin.h"

#include "DigiviewManager.h"
#include "SVBackend.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QFile>
#include <QtQml/QQmlApplicationEngine>

Q_APPLICATION_STATIC(CustomPlugin, _customPluginInstance);

CustomPlugin::CustomPlugin(QObject* parent)
    : QGCCorePlugin(parent)
    , _digiviewManager(new DigiviewManager(this))
    , _backend(new SVBackend(_digiviewManager, this))
{
}

QGCCorePlugin* CustomPlugin::instance()
{
    return _customPluginInstance();
}

SVBackend* SVBackend::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    return static_cast<CustomPlugin*>(CustomPlugin::instance())->backend();
}

QString CustomPlugin::firstRunPromptResource(int id) const
{
    if (id == kSVInitialWelcomePromptId) {
        return QStringLiteral("qrc:/qml/QGroundControl/SynclairVisionUI/Flyview/SVWelcomePrompt.qml");
    }

    return QGCCorePlugin::firstRunPromptResource(id);
}

QQmlApplicationEngine* CustomPlugin::createQmlApplicationEngine(QObject* parent)
{
    _qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    _urlInterceptor = new CustomOverrideInterceptor();
    _qmlEngine->addUrlInterceptor(_urlInterceptor);
    return _qmlEngine;
}

void CustomPlugin::destroyQmlApplicationEngine(QQmlApplicationEngine* qmlEngine)
{
    if (qmlEngine && qmlEngine == _qmlEngine) {
        qmlEngine->removeUrlInterceptor(_urlInterceptor);
        delete _urlInterceptor;
        _urlInterceptor = nullptr;
        _qmlEngine = nullptr;
    }

    QGCCorePlugin::destroyQmlApplicationEngine(qmlEngine);
}

QUrl CustomOverrideInterceptor::intercept(const QUrl& url, DataType type)
{
    if ((type == QmlFile || type == UrlString) && url.scheme() == QStringLiteral("qrc")) {
        const QString overrideResource = QStringLiteral(":/Custom%1").arg(url.path());
        if (QFile::exists(overrideResource)) {
            return QUrl(QStringLiteral("qrc%1").arg(overrideResource));
        }
    }

    return url;
}
