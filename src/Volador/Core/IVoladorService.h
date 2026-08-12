/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Enterprise Core Architecture
 *
 * Abstract Service Interface for Clean Architecture Component Services
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace Volador {

/**
 * @brief Abstract lifecycle status of a Volador GCS backend service.
 */
enum class ServiceStatus {
    Uninitialized,
    Initializing,
    Ready,
    Error,
    Stopped
};

/**
 * @brief IVoladorService defines the standard lifecycle contract for all independent
 * backend modules (Security, Database, Telemetry, Mission, Video, AI, Fleet, etc.).
 */
class IVoladorService : public QObject {
    Q_OBJECT

public:
    explicit IVoladorService(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IVoladorService() = default;

    /**
     * @brief Human-readable unique identifier for the service.
     */
    virtual QString serviceName() const = 0;

    /**
     * @brief Initialize the service resources, threads, and backend connections.
     * @return True if initialized successfully, false otherwise.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Gracefully shutdown and release all service resources.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Retrieve the current execution status of the service.
     */
    virtual ServiceStatus status() const = 0;

signals:
    /**
     * @brief Emitted whenever the service status changes.
     * @param status New service status.
     */
    void statusChanged(ServiceStatus status);

    /**
     * @brief Emitted when an operational error occurs within the service.
     * @param errorMessage Detailed error description.
     */
    void serviceError(const QString &errorMessage);
};

} // namespace Volador
