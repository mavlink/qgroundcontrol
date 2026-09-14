#include "CustomPlugin.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtQml/QQmlApplicationEngine>

#include "DigiviewManager.h"
#include "MultiVehicleManager.h"
#include "SVBackend.h"
#include "Vehicle.h"

Q_APPLICATION_STATIC(CustomPlugin, _customPluginInstance);

CustomPlugin::CustomPlugin(QObject* parent)
    : QGCCorePlugin(parent)
    , _digiviewManager(new DigiviewManager(this))
    , _backend(new SVBackend(_digiviewManager, this))
    , _toolBarIndicators(QGCCorePlugin::toolBarIndicators())
{
    _toolBarIndicators.prepend(QVariant::fromValue(
        QUrl(QStringLiteral("qrc:/qml/QGroundControl/SynclairVisionUI/Flyview/SVFlyViewToolbarIndicator.qml"))));

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded,
                   this, &CustomPlugin::_connectVehicle);
    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this, [this](Vehicle* vehicle) {
        if (vehicle) {
            const auto connection = _vehicleConnections.take(vehicle);
            QObject::disconnect(connection);
        }
    });
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

void CustomPlugin::prepareForClose()
{
    if (_closePreparationStarted) {
        return;
    }

    _closePreparationStarted = true;
    (void) connect(_digiviewManager, &DigiviewManager::transportConnectedChanged, this, [this] {
        if (!_digiviewManager->transportConnected() && !_digiviewManager->sessionRequested()) {
            _completePrepareForClose();
        }
    });
    (void) connect(_digiviewManager, &DigiviewManager::sessionRequestedChanged, this, [this] {
        if (!_digiviewManager->transportConnected() && !_digiviewManager->sessionRequested()) {
            _completePrepareForClose();
        }
    });

    _digiviewManager->stopRecording();
    _digiviewManager->disconnectFromHost();
    if (!_digiviewManager->transportConnected() && !_digiviewManager->sessionRequested()) {
        _completePrepareForClose();
    }
}

void CustomPlugin::_connectVehicle(Vehicle* vehicle)
{
    if (!vehicle || _vehicleConnections.contains(vehicle)) {
        return;
    }

    _vehicleConnections.insert(vehicle, connect(vehicle, &Vehicle::mavlinkMessageReceived,
                                                _digiviewManager, &DigiviewManager::receiveMavlinkMessage));
}

void CustomPlugin::_completePrepareForClose()
{
    if (_closePreparationCompleted) {
        return;
    }

    _closePreparationCompleted = true;
    emit prepareForCloseCompleted();
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
