/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Enterprise Core Architecture
 *
 * Central Engine Manager & Service Locator Implementation
 *
 ****************************************************************************/

#include "VoladorCore.h"
#include "../Database/VoladorDatabaseService.h"
#include "../Security/VoladorSecurityService.h"
#include "../UI/VoladorAuthViewModel.h"

#include <QtCore/QDebug>
#include <QtQml/QQmlContext>

namespace Volador {

VoladorCore *VoladorCore::_instance = nullptr;

VoladorCore::VoladorCore(QObject *parent)
    : QObject(parent)
{
    _instance = this;
}

VoladorCore::~VoladorCore() {
    shutdown();
    if (_instance == this) {
        _instance = nullptr;
    }
}

VoladorCore *VoladorCore::instance() {
    return _instance;
}

bool VoladorCore::initialize() {
    if (_initialized) {
        return true;
    }

    qInfo() << "[VoladorCore] Bootstrapping Volador Aerospace Enterprise GCS Engine...";

    // 1. Initialize SQLite Database Service
    _databaseService = new VoladorDatabaseService(this);
    if (!registerService(_databaseService) || !_databaseService->initialize()) {
        qCritical() << "[VoladorCore] Core initialization aborted: Database Service failed.";
        return false;
    }

    // 2. Initialize Security & Authentication Service
    _securityService = new VoladorSecurityService(_databaseService, this);
    if (!registerService(_securityService) || !_securityService->initialize()) {
        qCritical() << "[VoladorCore] Core initialization aborted: Security Service failed.";
        return false;
    }

    // 3. Create ViewModel & Controller instances
    _authViewModel = new VoladorAuthViewModel(_securityService, this);
    _startupController = new VoladorStartupController(this);

    _initialized = true;
    emit initializationChanged(_initialized);
    qInfo() << "[VoladorCore] Volador GCS Core Services initialized successfully.";
    return true;
}

void VoladorCore::shutdown() {
    if (!_initialized) {
        return;
    }

    qInfo() << "[VoladorCore] Shutting down core services...";
    for (auto it = _services.begin(); it != _services.end(); ++it) {
        if (it.value()) {
            it.value()->shutdown();
        }
    }
    _services.clear();

    _initialized = false;
    emit initializationChanged(_initialized);
    qInfo() << "[VoladorCore] Core engine shutdown complete.";
}

#include <QtQml/qqml.h>

void VoladorCore::registerQmlTypes(QQmlApplicationEngine *engine) {
    if (!engine) {
        qWarning() << "[VoladorCore] Cannot register QML types: engine pointer is null.";
        return;
    }

    qmlRegisterType<VoladorAuthViewModel>("Volador.UI", 1, 0, "VoladorAuthViewModel");
    qmlRegisterType<VoladorStartupController>("Volador.Core", 1, 0, "VoladorStartupController");

    QQmlContext *context = engine->rootContext();
    if (context) {
        context->setContextProperty(QStringLiteral("voladorCore"), this);
        context->setContextProperty(QStringLiteral("voladorAuth"), _authViewModel);
        context->setContextProperty(QStringLiteral("voladorStartup"), _startupController);
        qInfo() << "[VoladorCore] Volador GCS QML context properties registered (voladorCore, voladorAuth, voladorStartup).";
    }
}

bool VoladorCore::registerService(IVoladorService *service) {
    if (!service) {
        return false;
    }
    const QString name = service->serviceName();
    if (_services.contains(name)) {
        qWarning() << "[VoladorCore] Service already registered:" << name;
        return false;
    }
    _services.insert(name, service);
    emit serviceRegistered(name);
    return true;
}

IVoladorService *VoladorCore::getService(const QString &serviceName) const {
    return _services.value(serviceName, nullptr);
}

VoladorDatabaseService *VoladorCore::databaseService() const {
    return _databaseService;
}

VoladorSecurityService *VoladorCore::securityService() const {
    return _securityService;
}

} // namespace Volador
