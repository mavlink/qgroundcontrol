/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Security & Authentication Manager
 *
 * Role-Based Access Control (RBAC) & Secure User Session Service
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <memory>

#include "IVoladorService.h"

namespace Volador {

class VoladorDatabaseService;

/**
 * @brief User roles in Volador GCS.
 */
enum class UserRole {
    Viewer,
    Pilot,
    MissionPlanner,
    Administrator
};

/**
 * @brief Granular operational permissions.
 */
enum class Permission {
    ViewTelemetry,
    ArmVehicle,
    DisarmVehicle,
    UploadMission,
    EditMission,
    SystemSettings,
    UserManagement,
    PayloadControl
};

/**
 * @brief Struct representing an active authenticated session.
 */
struct UserSession {
    bool isValid{false};
    int userId{-1};
    QString username;
    QString fullName;
    UserRole role{UserRole::Viewer};
    QString sessionToken;
    QDateTime loginTime;
};

/**
 * @brief VoladorSecurityService provides user authentication, RBAC permission checks,
 * session management, and encrypted preference handling for Volador GCS.
 */
class VoladorSecurityService : public IVoladorService {
    Q_OBJECT

public:
    explicit VoladorSecurityService(VoladorDatabaseService *dbService, QObject *parent = nullptr);
    ~VoladorSecurityService() override;

    // IVoladorService implementation
    QString serviceName() const override { return QStringLiteral("VoladorSecurityService"); }
    bool initialize() override;
    void shutdown() override;
    ServiceStatus status() const override { return _status; }

    /**
     * @brief Authenticate user against persistent security database.
     * @param username Provided username.
     * @param password Provided password.
     * @return True if authentication succeeds.
     */
    bool authenticate(const QString &username, const QString &password);

    /**
     * @brief Terminate current user session.
     */
    void logout();

    /**
     * @brief Check if a valid user is currently logged in.
     */
    bool isAuthenticated() const;

    /**
     * @brief Check if active session holds specific operational permission.
     * @param perm Permission to evaluate.
     */
    bool hasPermission(Permission perm) const;

    /**
     * @brief Active user session details.
     */
    UserSession activeSession() const;

    /**
     * @brief Convert UserRole enum to human-readable string.
     */
    static QString roleToString(UserRole role);

    /**
     * @brief Parse string to UserRole enum.
     */
    static UserRole stringToRole(const QString &roleStr);

signals:
    void authenticationStateChanged(bool authenticated, const QString &username, const QString &role);
    void authenticationFailed(const QString &reason);

private:
    QString hashPassword(const QString &password, const QString &salt) const;

    ServiceStatus _status{ServiceStatus::Uninitialized};
    VoladorDatabaseService *_dbService{nullptr};
    UserSession _activeSession;
};

} // namespace Volador
