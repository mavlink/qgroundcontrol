#include "FoxFourPlugin.h"
#include "QGCLoggingCategory.h"
#include "QGCPalette.h"
#include "BrandImageSettings.h"

#include <QtCore/QApplicationStatic>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlFile>

QGC_LOGGING_CATEGORY(CustomLog, "FoxFour.Plugin")

Q_APPLICATION_STATIC(FoxFourPlugin, _customPluginInstance);

FoxFourPlugin::FoxFourPlugin(QObject *parent)
    : QGCCorePlugin(parent)
    , _options(new QGCOptions(this))
    , _parameterSetter(new ParameterSetter(this))
{
    _version = QString(QGC_CUSTOM_VERSION);
    _showAdvancedUI = true;
    (void) connect(this, &FoxFourPlugin::showAdvancedUIChanged, this, &FoxFourPlugin::_advancedChanged);
}

QGCCorePlugin *FoxFourPlugin::instance()
{
    return _customPluginInstance();
}

void FoxFourPlugin::cleanup()
{
    if (_qmlEngine) {
        _qmlEngine->removeUrlInterceptor(_selector);
    }

    delete _selector;
}

void FoxFourPlugin::_advancedChanged(bool changed)
{
    // Firmware Upgrade page is only show in Advanced mode
    emit _options->showFirmwareUpgradeChanged(changed);
}

bool FoxFourPlugin::overrideSettingsGroupVisibility(const QString &name)
{
    // We have set up our own specific brand imaging.
    // Hide the brand image settings such that the end user can't change it.
    if (name == BrandImageSettings::name) {
        return false;
    }

    return true;
}

VideoReceiver *FoxFourPlugin::createVideoReceiver(QObject *parent)
{
#ifdef QGC_GST_STREAMING
    return new FoxFourGstVideoReceiver(parent);
#elif defined(QGC_QT_STREAMING)
    return QtMultimediaReceiver::createVideoReceiver(parent);
#else
    return nullptr;
#endif
}

QQmlApplicationEngine* FoxFourPlugin::createQmlApplicationEngine(QObject* parent)
{
    _qmlEngine = QGCCorePlugin::createQmlApplicationEngine(parent);
    // TODO: Investigate _qmlEngine->setExtraSelectors({"custom"})
    _selector = new CustomOverrideInterceptor();
    _qmlEngine->addUrlInterceptor(_selector);

    return _qmlEngine;
}

/*===========================================================================*/

CustomOverrideInterceptor::CustomOverrideInterceptor()
    : QQmlAbstractUrlInterceptor()
{

}

QUrl CustomOverrideInterceptor::intercept(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type)
{
    switch (type) {
    using DataType = QQmlAbstractUrlInterceptor::DataType;
    case DataType::QmlFile:
    case DataType::UrlString:
        if (url.scheme() == QStringLiteral("qrc")) {
            const QString origPath = url.path();
            const QString overrideRes = QStringLiteral(":/Custom%1").arg(origPath);

            if (QFile::exists(overrideRes)) {
                // if (overrideRes.endsWith("qml")) {
                    qCDebug(CustomLog) << "Overiding: "<< origPath <<" with " << overrideRes;
                // }
                const QString relPath = overrideRes.mid(2);
                QUrl result;
                result.setScheme(QStringLiteral("qrc"));
                result.setPath('/' + relPath);
                return result;
            }
        }
        break;
    default:
        break;
    }

    return url;
}
