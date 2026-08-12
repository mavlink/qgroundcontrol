/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station - Database Manager
 *
 * SQLite Database Service for Local Data Persistence
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMutex>
#include <QtSql/QSqlDatabase>
#include <memory>

#include "IVoladorService.h"

namespace Volador {

/**
 * @brief VoladorDatabaseService manages the SQLite database lifecycle, schema updates,
 * thread-safe queries, and data caching for telemetry, missions, and security credentials.
 */
class VoladorDatabaseService : public IVoladorService {
    Q_OBJECT

public:
    explicit VoladorDatabaseService(QObject *parent = nullptr);
    ~VoladorDatabaseService() override;

    // IVoladorService implementation
    QString serviceName() const override { return QStringLiteral("VoladorDatabaseService"); }
    bool initialize() override;
    void shutdown() override;
    ServiceStatus status() const override { return _status; }

    /**
     * @brief Get an operational handle to the database connection.
     */
    QSqlDatabase database();

    /**
     * @brief Executes a custom SQL statement safely.
     * @param queryString SQL command.
     * @return True on success.
     */
    bool executeQuery(const QString &queryString);

    /**
     * @brief Seeds default system user accounts if database is fresh.
     */
    bool seedDefaultUsers();

private:
    bool createTables();

    ServiceStatus _status{ServiceStatus::Uninitialized};
    QString _dbPath;
    QString _connectionName;
    mutable QMutex _dbMutex;
};

} // namespace Volador
