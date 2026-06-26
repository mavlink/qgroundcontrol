#include "QGCTileDatabaseSchema.h"

#include <QtCore/QDateTime>
#include <QtCore/QLatin1String>
#include <QtCore/QLatin1StringView>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCSqlHelper.h"
#include "QGCTileCacheDatabase.h"

QGC_LOGGING_CATEGORY(QGCTileDatabaseSchemaLog, "QtLocationPlugin.QGCTileDatabaseSchema")

namespace QGCTileDatabaseSchema {

bool dropSchemaTables(QSqlDatabase db)
{
    QGCSqlHelper::Transaction txn(db);
    if (!txn.ok()) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to start transaction for table drops";
        return false;
    }
    QSqlQuery query(db);
    if (!query.exec("DROP TABLE IF EXISTS TilesDownload") || !query.exec("DROP TABLE IF EXISTS SetTiles") ||
        !query.exec("DROP TABLE IF EXISTS Tiles") || !query.exec("DROP TABLE IF EXISTS TileSets")) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to drop tables:" << query.lastError().text();
        return false;
    }
    if (!txn.commit()) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to commit table drops";
        return false;
    }
    return true;
}

bool migrateSchema(QSqlDatabase db, int fromVersion)
{
    QGCSqlHelper::Transaction txn(db);
    if (!txn.ok()) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to start transaction for schema migration";
        return false;
    }

    if (fromVersion < 2) {
        // v1 -> v2: add HTTP cache-validation columns to Tiles. Existing rows
        // get NULL/0 and simply revalidate on next fetch.
        QSet<QString> columns;
        QSqlQuery info(db);
        if (info.exec(QStringLiteral("PRAGMA table_info(Tiles)"))) {
            while (info.next()) {
                columns.insert(info.value(1).toString());
            }
        }

        struct ColumnDef
        {
            const char* name;
            const char* ddl;
        };

        static const ColumnDef newColumns[] = {
            {"etag", "ALTER TABLE Tiles ADD COLUMN etag TEXT"},
            {"lastModified", "ALTER TABLE Tiles ADD COLUMN lastModified TEXT"},
            {"expiresAt", "ALTER TABLE Tiles ADD COLUMN expiresAt INTEGER DEFAULT 0"},
        };
        for (const ColumnDef& col : newColumns) {
            if (columns.contains(QLatin1String(col.name))) {
                continue;
            }
            QSqlQuery alter(db);
            if (!alter.exec(QLatin1String(col.ddl))) {
                qCWarning(QGCTileDatabaseSchemaLog)
                    << "Failed to add column" << col.name << ":" << alter.lastError().text();
                return false;
            }
        }
    }

    if (fromVersion < 3) {
        // v2 -> v3: add LRU access tracking + per-tile must-revalidate flag.
        QSet<QString> columns;
        QSqlQuery info(db);
        if (info.exec(QStringLiteral("PRAGMA table_info(Tiles)"))) {
            while (info.next()) {
                columns.insert(info.value(1).toString());
            }
        }

        if (!columns.contains(QStringLiteral("accessed"))) {
            QSqlQuery alter(db);
            if (!alter.exec(QStringLiteral("ALTER TABLE Tiles ADD COLUMN accessed INTEGER NOT NULL DEFAULT 0"))) {
                qCWarning(QGCTileDatabaseSchemaLog) << "Failed to add column accessed:" << alter.lastError().text();
                return false;
            }
            // Seed LRU order from insertion date so pre-v3 tiles are not all equal.
            if (!alter.exec(QStringLiteral("UPDATE Tiles SET accessed = date"))) {
                qCWarning(QGCTileDatabaseSchemaLog) << "Failed to backfill accessed:" << alter.lastError().text();
                return false;
            }
        }

        if (!columns.contains(QStringLiteral("mustRevalidate"))) {
            QSqlQuery alter(db);
            if (!alter.exec(QStringLiteral("ALTER TABLE Tiles ADD COLUMN mustRevalidate INTEGER NOT NULL DEFAULT 0"))) {
                qCWarning(QGCTileDatabaseSchemaLog)
                    << "Failed to add column mustRevalidate:" << alter.lastError().text();
                return false;
            }
        }

        QSqlQuery index(db);
        if (!index.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tiles_accessed ON Tiles(accessed)"))) {
            qCWarning(QGCTileDatabaseSchemaLog) << "Failed to create idx_tiles_accessed:" << index.lastError().text();
            return false;
        }
    }

    if (fromVersion < 4) {
        // v3 -> v4: covering index on (accessed, size) so pruneCache's LRU scan +
        // size sum run index-only, without faulting BLOB pages into the cache.
        QSqlQuery index(db);
        if (!index.exec(
                QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tiles_accessed_size ON Tiles(accessed, size)"))) {
            qCWarning(QGCTileDatabaseSchemaLog)
                << "Failed to create idx_tiles_accessed_size:" << index.lastError().text();
            return false;
        }
    }

    if (fromVersion < 5) {
        // v4 -> v5: persist the stable provider-name string. The old int `type` is a
        // positional registry index that mis-attributes tiles if providers are reordered.
        auto addTypeStrColumn = [&db](const char* table) -> bool {
            QSet<QString> columns;
            QSqlQuery info(db);
            if (info.exec(QStringLiteral("PRAGMA table_info(%1)").arg(QLatin1String(table)))) {
                while (info.next()) {
                    columns.insert(info.value(1).toString());
                }
            }
            if (columns.contains(QStringLiteral("typeStr"))) {
                return true;
            }
            QSqlQuery alter(db);
            if (!alter.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN typeStr TEXT").arg(QLatin1String(table)))) {
                qCWarning(QGCTileDatabaseSchemaLog)
                    << "Failed to add typeStr to" << table << ":" << alter.lastError().text();
                return false;
            }
            return true;
        };

        auto backfillTypeStr = [&db](const char* table) -> bool {
            QList<int> mapIds;
            QSqlQuery distinct(db);
            if (distinct.exec(QStringLiteral("SELECT DISTINCT type FROM %1 WHERE typeStr IS NULL OR typeStr = ''")
                                  .arg(QLatin1String(table)))) {
                while (distinct.next()) {
                    mapIds.append(distinct.value(0).toInt());
                }
            }
            QSqlQuery update(db);
            if (!update.prepare(QStringLiteral("UPDATE %1 SET typeStr = ? WHERE type = ? AND (typeStr IS NULL OR "
                                               "typeStr = '')")
                                    .arg(QLatin1String(table)))) {
                return false;
            }
            for (const int mapId : mapIds) {
                // Provider mapIds start at 1; skip 0/-1 sentinels so backfill doesn't log a
                // spurious "provider not found". Those rows keep NULL typeStr and revalidate.
                if (mapId < 1) {
                    continue;
                }
                const QString name = UrlFactory::getProviderTypeFromQtMapId(mapId);
                if (name.isEmpty()) {
                    continue;
                }
                update.bindValue(0, name);
                update.bindValue(1, mapId);
                if (!update.exec()) {
                    qCWarning(QGCTileDatabaseSchemaLog)
                        << "Failed to backfill typeStr on" << table << ":" << update.lastError().text();
                    return false;
                }
            }
            return true;
        };

        if (!addTypeStrColumn("Tiles") || !addTypeStrColumn("TilesDownload")) {
            return false;
        }
        if (!backfillTypeStr("Tiles") || !backfillTypeStr("TilesDownload") || !backfillTypeStr("TileSets")) {
            return false;
        }
    }

    if (!QGCSqlHelper::setUserVersion(db, QGCTileCacheDatabase::kSchemaVersion)) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to set schema version after migration";
        return false;
    }

    if (!txn.commit()) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to commit schema migration";
        return false;
    }

    qCDebug(QGCTileDatabaseSchemaLog) << "Migrated tile cache schema from version" << fromVersion << "to"
                                      << QGCTileCacheDatabase::kSchemaVersion;
    return true;
}

bool createSchema(QSqlDatabase db, bool createDefault)
{
    // applySqlitePragmas (in connectDB / ScopedConnection ctor) already
    // enabled foreign_keys; nothing to redo here.
    QSqlQuery query(db);

    if (!query.exec("CREATE TABLE IF NOT EXISTS Tiles ("
                    "tileID INTEGER PRIMARY KEY NOT NULL, "
                    "hash TEXT NOT NULL UNIQUE, "
                    "format TEXT NOT NULL, "
                    "tile BLOB NULL, "
                    "size INTEGER, "
                    "typeStr TEXT, "
                    "date INTEGER DEFAULT 0, "
                    "etag TEXT, "
                    "lastModified TEXT, "
                    "expiresAt INTEGER DEFAULT 0, "
                    "accessed INTEGER NOT NULL DEFAULT 0, "
                    "mustRevalidate INTEGER NOT NULL DEFAULT 0)")) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Map Cache SQL error (create Tiles db):" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS TileSets ("
                    "setID INTEGER PRIMARY KEY NOT NULL, "
                    "name TEXT NOT NULL UNIQUE, "
                    "typeStr TEXT, "
                    "topleftLat REAL DEFAULT 0.0, "
                    "topleftLon REAL DEFAULT 0.0, "
                    "bottomRightLat REAL DEFAULT 0.0, "
                    "bottomRightLon REAL DEFAULT 0.0, "
                    "minZoom INTEGER DEFAULT 3, "
                    "maxZoom INTEGER DEFAULT 3, "
                    "numTiles INTEGER DEFAULT 0, "
                    "defaultSet INTEGER DEFAULT 0, "
                    "date INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Map Cache SQL error (create TileSets db):" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS SetTiles ("
                    "setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
                    "tileID INTEGER NOT NULL REFERENCES Tiles(tileID) ON DELETE CASCADE)")) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Map Cache SQL error (create SetTiles db):" << query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS TilesDownload ("
                    "setID INTEGER NOT NULL REFERENCES TileSets(setID) ON DELETE CASCADE, "
                    "hash TEXT NOT NULL, "
                    "typeStr TEXT, "
                    "x INTEGER, "
                    "y INTEGER, "
                    "z INTEGER, "
                    "state INTEGER DEFAULT 0)")) {
        qCWarning(QGCTileDatabaseSchemaLog)
            << "Map Cache SQL error (create TilesDownload db):" << query.lastError().text();
        return false;
    }

    static const char* indexStatements[] = {
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_settiles_unique ON SetTiles(tileID, setID)",
        "CREATE INDEX IF NOT EXISTS idx_settiles_setid ON SetTiles(setID)",
        "CREATE INDEX IF NOT EXISTS idx_settiles_tileid ON SetTiles(tileID)",
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_tilesdownload_setid_hash ON TilesDownload(setID, hash)",
        "CREATE INDEX IF NOT EXISTS idx_tilesdownload_setid_state ON TilesDownload(setID, state)",
        "CREATE INDEX IF NOT EXISTS idx_tiles_date ON Tiles(date)",
        "CREATE INDEX IF NOT EXISTS idx_tiles_accessed ON Tiles(accessed)",
        "CREATE INDEX IF NOT EXISTS idx_tiles_accessed_size ON Tiles(accessed, size)",
        "CREATE INDEX IF NOT EXISTS idx_tilesets_default ON TileSets(defaultSet)",
    };
    for (const char* sql : indexStatements) {
        if (!query.exec(QLatin1String(sql))) {
            qCWarning(QGCTileDatabaseSchemaLog) << "Failed to create index:" << sql << query.lastError().text();
        }
    }

    if (!QGCSqlHelper::setUserVersion(db, QGCTileCacheDatabase::kSchemaVersion)) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to set schema version";
    }

    if (!createDefault) {
        return true;
    }

    if (!query.prepare("SELECT name FROM TileSets WHERE name = ?")) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Map Cache SQL error (prepare default set check):" << db.lastError();
        return false;
    }
    query.addBindValue(QStringLiteral("Default Tile Set"));
    if (!query.exec()) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Map Cache SQL error (Looking for default tile set):" << db.lastError();
        return true;
    }
    if (query.next()) {
        return true;
    }

    if (!query.prepare("INSERT INTO TileSets(name, defaultSet, date) VALUES(?, ?, ?)")) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Map Cache SQL error (prepare default tile set):" << db.lastError();
        return false;
    }
    query.addBindValue(QStringLiteral("Default Tile Set"));
    query.addBindValue(1);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec()) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Map Cache SQL error (Creating default tile set):" << db.lastError();
        return false;
    }

    return true;
}

bool checkSchemaVersion(QSqlDatabase db, bool* didReset)
{
    const auto current = QGCSqlHelper::userVersion(db);
    if (!current) {
        qCWarning(QGCTileDatabaseSchemaLog) << "Failed to read schema version";
        return false;
    }

    const int version = *current;
    if (version == QGCTileCacheDatabase::kSchemaVersion) {
        return true;
    }

    if (version == 0) {
        // Either a fresh database or a legacy database created before versioning.
        // Check for existing data — if Tiles table exists with rows, it's legacy.
        // Legacy DBs stored map type as text; migration is not supported so the cache is rebuilt.
        bool isLegacy = false;
        {
            QSqlQuery query(db);
            isLegacy = query.exec("SELECT COUNT(*) FROM Tiles") && query.next() && (query.value(0).toInt() > 0);
            // In WAL mode the live SELECT cursor holds a read lock; finalize it before
            // the DROPs or SQLite reports "database table is locked".
            query.finish();
        }
        if (isLegacy) {
            qCWarning(QGCTileDatabaseSchemaLog)
                << "Legacy database detected (no schema version). Discarding cached tiles and rebuilding.";
            if (didReset) {
                *didReset = true;
            }
            if (!dropSchemaTables(db)) {
                return false;
            }
        }
        return true;
    }

    if ((version > 0) && (version < QGCTileCacheDatabase::kSchemaVersion)) {
        if (migrateSchema(db, version)) {
            return true;
        }
        qCWarning(QGCTileDatabaseSchemaLog) << "Schema migration from version" << version << "failed. Resetting cache.";
    } else {
        qCWarning(QGCTileDatabaseSchemaLog) << "Unknown schema version" << version << "(expected"
                                            << QGCTileCacheDatabase::kSchemaVersion << "). Resetting cache.";
    }

    if (didReset) {
        *didReset = true;
    }
    return dropSchemaTables(db);
}

}  // namespace QGCTileDatabaseSchema
