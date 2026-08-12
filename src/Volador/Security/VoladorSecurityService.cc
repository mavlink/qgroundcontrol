/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Security & Authentication Manager
 *
 * Role-Based Access Control (RBAC) & Secure User Session Implementation
 *
 ****************************************************************************/

#include "VoladorSecurityService.h"
#include "../Database/VoladorDatabaseService.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QUuid>
#include <QtCore/QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

namespace Volador {

VoladorSecurityService::VoladorSecurityService(VoladorDatabaseService *dbService, QObject *parent)
    : IVoladorService(parent)
    , _dbService(dbService)
{
}

VoladorSecurityService::~VoladorSecurityService() {
    shutdown();
}

bool VoladorSecurityService::initialize() {
    _status = ServiceStatus::Initializing;
    emit statusChanged(_status);

    if (!_dbService || _dbService->status() != ServiceStatus::Ready) {
        qCritical() << "[VoladorSecurityService] Dependency error: VoladorDatabaseService is not ready.";
        _status = ServiceStatus::Error;
        emit statusChanged(_status);
        return false;
    }

    _status = ServiceStatus::Ready;
    emit statusChanged(_status);
    qInfo() << "[VoladorSecurityService] Security & Authentication Service initialized successfully.";
    return true;
}

void VoladorSecurityService::shutdown() {
    if (_status == ServiceStatus::Stopped) {
        return;
    }
    logout();
    _status = ServiceStatus::Stopped;
    emit statusChanged(_status);
    qInfo() << "[VoladorSecurityService] Security service shut down cleanly.";
}

bool VoladorSecurityService::authenticate(const QString &username, const QString &password) {
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        emit authenticationFailed(QStringLiteral("Username and password cannot be empty."));
        return false;
    }

    if (!_dbService) {
        emit authenticationFailed(QStringLiteral("Database service unavailable."));
        return false;
    }

    QSqlDatabase db = _dbService->database();
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, username, password_hash, salt, role, full_name "
        "FROM users WHERE username = :username LIMIT 1;"
    ));
    query.bindValue(QStringLiteral(":username"), username.trimmed());

    if (!query.exec() || !query.next()) {
        qWarning() << "[VoladorSecurityService] Invalid login attempt for user:" << username;
        emit authenticationFailed(QStringLiteral("Invalid username or password."));
        return false;
    }

    const int id = query.value(QStringLiteral("id")).toInt();
    const QString dbUsername = query.value(QStringLiteral("username")).toString();
    const QString dbHash = query.value(QStringLiteral("password_hash")).toString();
    const QString dbSalt = query.value(QStringLiteral("salt")).toString();
    const QString dbRole = query.value(QStringLiteral("role")).toString();
    const QString dbFullName = query.value(QStringLiteral("full_name")).toString();

    const QString computedHash = hashPassword(password, dbSalt);
    if (computedHash != dbHash) {
        qWarning() << "[VoladorSecurityService] Password mismatch for user:" << username;
        emit authenticationFailed(QStringLiteral("Invalid username or password."));
        return false;
    }

    _activeSession.isValid = true;
    _activeSession.userId = id;
    _activeSession.username = dbUsername;
    _activeSession.fullName = dbFullName;
    _activeSession.role = stringToRole(dbRole);
    _activeSession.sessionToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _activeSession.loginTime = QDateTime::currentDateTime();

    qInfo() << "[VoladorSecurityService] User" << dbUsername << "authenticated successfully with role:" << dbRole;

    emit authenticationStateChanged(true, _activeSession.username, roleToString(_activeSession.role));
    return true;
}

void VoladorSecurityService::logout() {
    if (_activeSession.isValid) {
        const QString username = _activeSession.username;
        _activeSession = UserSession();
        qInfo() << "[VoladorSecurityService] User" << username << "logged out.";
        emit authenticationStateChanged(false, QString(), QString());
    }
}

bool VoladorSecurityService::isAuthenticated() const {
    return _activeSession.isValid;
}

bool VoladorSecurityService::hasPermission(Permission perm) const {
    if (!_activeSession.isValid) {
        return false;
    }

    switch (_activeSession.role) {
    case UserRole::Administrator:
        return true; // Full access

    case UserRole::MissionPlanner:
        return perm == Permission::ViewTelemetry ||
               perm == Permission::UploadMission ||
               perm == Permission::EditMission;

    case UserRole::Pilot:
        return perm == Permission::ViewTelemetry ||
               perm == Permission::ArmVehicle ||
               perm == Permission::DisarmVehicle ||
               perm == Permission::UploadMission ||
               perm == Permission::PayloadControl;

    case UserRole::Viewer:
    default:
        return perm == Permission::ViewTelemetry;
    }
}

UserSession VoladorSecurityService::activeSession() const {
    return _activeSession;
}

QString VoladorSecurityService::roleToString(UserRole role) {
    switch (role) {
    case UserRole::Administrator:   return QStringLiteral("Administrator");
    case UserRole::MissionPlanner: return QStringLiteral("MissionPlanner");
    case UserRole::Pilot:          return QStringLiteral("Pilot");
    case UserRole::Viewer:         return QStringLiteral("Viewer");
    }
    return QStringLiteral("Viewer");
}

UserRole VoladorSecurityService::stringToRole(const QString &roleStr) {
    if (roleStr.compare(QStringLiteral("Administrator"), Qt::CaseInsensitive) == 0)   return UserRole::Administrator;
    if (roleStr.compare(QStringLiteral("MissionPlanner"), Qt::CaseInsensitive) == 0) return UserRole::MissionPlanner;
    if (roleStr.compare(QStringLiteral("Pilot"), Qt::CaseInsensitive) == 0)          return UserRole::Pilot;
    return UserRole::Viewer;
}

QString VoladorSecurityService::hashPassword(const QString &password, const QString &salt) const {
    QByteArray combined = (password + salt).toUtf8();
    return QString::fromUtf8(QCryptographicHash::hash(combined, QCryptographicHash::Sha256).toHex());
}

} // namespace Volador
