/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Database Manager
 *
 * SQLite Database Service Implementation
 *
 ****************************************************************************/

#include "VoladorDatabaseService.h"

#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QCryptographicHash>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

namespace Volador {

VoladorDatabaseService::VoladorDatabaseService(QObject *parent)
    : IVoladorService(parent)
    , _connectionName(QStringLiteral("VoladorGCSConnection"))
{
}

VoladorDatabaseService::~VoladorDatabaseService() {
    shutdown();
}

bool VoladorDatabaseService::initialize() {
    QMutexLocker locker(&_dbMutex);
    _status = ServiceStatus::Initializing;
    emit statusChanged(_status);

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(dataDir);
    }

    _dbPath = dir.filePath(QStringLiteral("volador_gcs.db"));

    if (QSqlDatabase::contains(_connectionName)) {
        QSqlDatabase::removeDatabase(_connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), _connectionName);
    db.setDatabaseName(_dbPath);

    if (!db.open()) {
        const QString err = QStringLiteral("Failed to open SQLite database at %1: %2")
                                .arg(_dbPath, db.lastError().text());
        qCritical() << "[VoladorDatabaseService]" << err;
        _status = ServiceStatus::Error;
        emit statusChanged(_status);
        emit serviceError(err);
        return false;
    }

    qInfo() << "[VoladorDatabaseService] SQLite database opened successfully at" << _dbPath;

    if (!createTables()) {
        _status = ServiceStatus::Error;
        emit statusChanged(_status);
        return false;
    }

    _status = ServiceStatus::Ready;
    emit statusChanged(_status);
    return true;
}

void VoladorDatabaseService::shutdown() {
    QMutexLocker locker(&_dbMutex);
    if (_status == ServiceStatus::Stopped) {
        return;
    }

    if (QSqlDatabase::contains(_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(_connectionName, false);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(_connectionName);
    }

    _status = ServiceStatus::Stopped;
    emit statusChanged(_status);
    qInfo() << "[VoladorDatabaseService] Database service shut down cleanly.";
}

QSqlDatabase VoladorDatabaseService::database() {
    return QSqlDatabase::database(_connectionName);
}

bool VoladorDatabaseService::executeQuery(const QString &queryString) {
    QMutexLocker locker(&_dbMutex);
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        qWarning() << "[VoladorDatabaseService] Query attempt on closed DB connection.";
        return false;
    }

    QSqlQuery query(db);
    if (!query.exec(queryString)) {
        qWarning() << "[VoladorDatabaseService] Query failed:" << query.lastError().text() << "Query:" << queryString;
        return false;
    }
    return true;
}

bool VoladorDatabaseService::createTables() {
    QSqlDatabase db = database();
    QSqlQuery query(db);

    // Users & RBAC Table
    const QString createUsersTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT UNIQUE NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  salt TEXT NOT NULL,"
        "  role TEXT NOT NULL,"
        "  full_name TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
    );

    if (!query.exec(createUsersTable)) {
        qCritical() << "[VoladorDatabaseService] Failed to create users table:" << query.lastError().text();
        return false;
    }

    // Telemetry Cache Table
    const QString createTelemetryTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS telemetry_cache ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  vehicle_id INT NOT NULL,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  latitude REAL,"
        "  longitude REAL,"
        "  altitude REAL,"
        "  heading REAL,"
        "  battery_percentage INT,"
        "  flight_mode TEXT"
        ");"
    );

    if (!query.exec(createTelemetryTable)) {
        qCritical() << "[VoladorDatabaseService] Failed to create telemetry_cache table:" << query.lastError().text();
        return false;
    }

    // Flight Log Metadata Table
    const QString createLogsTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS flight_logs ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  log_name TEXT NOT NULL,"
        "  start_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  end_time DATETIME,"
        "  duration_seconds INT,"
        "  vehicle_id INT,"
        "  file_path TEXT"
        ");"
    );

    if (!query.exec(createLogsTable)) {
        qCritical() << "[VoladorDatabaseService] Failed to create flight_logs table:" << query.lastError().text();
        return false;
    }

    // Mission History Table
    const QString createMissionTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS mission_history ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  mission_name TEXT NOT NULL,"
        "  created_by TEXT,"
        "  waypoint_count INT,"
        "  json_data TEXT,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
    );

    if (!query.exec(createMissionTable)) {
        qCritical() << "[VoladorDatabaseService] Failed to create mission_history table:" << query.lastError().text();
        return false;
    }

    return seedDefaultUsers();
}

bool VoladorDatabaseService::seedDefaultUsers() {
    QSqlDatabase db = database();
    QSqlQuery query(db);

    query.exec(QStringLiteral("SELECT COUNT(*) FROM users;"));
    if (query.next() && query.value(0).toInt() > 0) {
        // Users already exist
        return true;
    }

    qInfo() << "[VoladorDatabaseService] Seeding default Volador GCS user accounts...";

    // Generate salted SHA-256 password hash helper
    auto hashPassword = [](const QString &password, const QString &salt) {
        QByteArray combined = (password + salt).toUtf8();
        return QString::fromUtf8(QCryptographicHash::hash(combined, QCryptographicHash::Sha256).toHex());
    };

    struct DefaultUser {
        QString username;
        QString password;
        QString role;
        QString fullName;
    };

    const QList<DefaultUser> defaults = {
        { QStringLiteral("admin"),   QStringLiteral("admin123"),   QStringLiteral("Administrator"), QStringLiteral("Lead System Administrator") },
        { QStringLiteral("planner"), QStringLiteral("planner123"), QStringLiteral("MissionPlanner"), QStringLiteral("Senior Mission Planner") },
        { QStringLiteral("pilot"),   QStringLiteral("pilot123"),   QStringLiteral("Pilot"),          QStringLiteral("Chief Flight Pilot") }
    };

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO users (username, password_hash, salt, role, full_name) "
        "VALUES (:username, :hash, :salt, :role, :fullName);"
    ));

    for (const auto &user : defaults) {
        const QString salt = QStringLiteral("VoladorSalt_%1").arg(user.username);
        const QString hash = hashPassword(user.password, salt);

        insertQuery.bindValue(QStringLiteral(":username"), user.username);
        insertQuery.bindValue(QStringLiteral(":hash"), hash);
        insertQuery.bindValue(QStringLiteral(":salt"), salt);
        insertQuery.bindValue(QStringLiteral(":role"), user.role);
        insertQuery.bindValue(QStringLiteral(":fullName"), user.fullName);

        if (!insertQuery.exec()) {
            qWarning() << "[VoladorDatabaseService] Failed to seed user:" << user.username << insertQuery.lastError().text();
        }
    }

    qInfo() << "[VoladorDatabaseService] Default user accounts seeded successfully.";
    return true;
}

} // namespace Volador
