/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Enterprise Core Architecture
 *
 * Central Engine Manager & Service Locator Singleton
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtQml/QQmlApplicationEngine>
#include <memory>

#include "IVoladorService.h"
#include "VoladorVersion.h"
#include "VoladorStartupController.h"
#include "../UI/VoladorAuthViewModel.h"

namespace Volador {

class VoladorDatabaseService;
class VoladorSecurityService;

/**
 * @brief VoladorCore is the main central engine singleton for Volador GCS.
 * It manages service discovery, service lifecycle initialization, and QML engine registration.
 */
class VoladorCore : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isInitialized READ isInitialized NOTIFY initializationChanged)
    Q_PROPERTY(VoladorAuthViewModel* authViewModel READ authViewModel CONSTANT)
    Q_PROPERTY(VoladorStartupController* startupController READ startupController CONSTANT)
    Q_PROPERTY(QString appName READ appName CONSTANT)
    Q_PROPERTY(QString appShortName READ appShortName CONSTANT)
    Q_PROPERTY(QString company READ company CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(QString website READ website CONSTANT)
    Q_PROPERTY(QString copyright READ copyright CONSTANT)
    Q_PROPERTY(QString versionString READ versionString CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

public:
    explicit VoladorCore(QObject *parent = nullptr);
    ~VoladorCore() override;

    VoladorAuthViewModel* authViewModel() const { return _authViewModel; }
    VoladorStartupController* startupController() const { return _startupController; }

    QString appName() const { return Version::appName(); }
    QString appShortName() const { return Version::appShortName(); }
    QString company() const { return Version::company(); }
    QString description() const { return Version::description(); }
    QString website() const { return Version::website(); }
    QString copyright() const { return Version::copyright(); }
    QString versionString() const { return Version::versionString(); }
    QString qtVersion() const { return QStringLiteral(QT_VERSION_STR); }

    /**
     * @brief Singleton instance accessor.
     */
    static VoladorCore *instance();

    /**
     * @brief Bootstraps and initializes all core Volador GCS services.
     * @return True if all mandatory services started successfully.
     */
    bool initialize();

    /**
     * @brief Gracefully shuts down all registered services.
     */
    void shutdown();

    /**
     * @brief Registers QML types and context properties for UI integration.
     * @param engine Pointer to the QQmlApplicationEngine.
     */
    void registerQmlTypes(QQmlApplicationEngine *engine);

    /**
     * @brief Register a modular service in the central service locator.
     * @param service Pointer to the service instance.
     */
    bool registerService(IVoladorService *service);

    /**
     * @brief Retrieve a registered service by its unique name.
     * @param serviceName The string key identifying the service.
     */
    IVoladorService *getService(const QString &serviceName) const;

    /**
     * @brief Convenient accessor for Database Service.
     */
    VoladorDatabaseService *databaseService() const;

    /**
     * @brief Convenient accessor for Security Service.
     */
    VoladorSecurityService *securityService() const;

    /**
     * @brief Check whether core services are fully initialized.
     */
    bool isInitialized() const { return _initialized; }

signals:
    void initializationChanged(bool initialized);
    void serviceRegistered(const QString &serviceName);

private:
    static VoladorCore *_instance;

    bool _initialized{false};
    QMap<QString, IVoladorService*> _services;

    VoladorDatabaseService *_databaseService{nullptr};
    VoladorSecurityService *_securityService{nullptr};
    VoladorAuthViewModel *_authViewModel{nullptr};
    VoladorStartupController *_startupController{nullptr};
};

} // namespace Volador
