#pragma once

class QSqlDatabase;

// Schema DDL and version migration for the tile cache database, split out of
// QGCTileCacheDatabase so create/migrate/reset can be read and unit-tested
// independently of tile CRUD, stats and import/export. Every function operates
// on the caller's already-open QSqlDatabase connection and manages its own
// transaction; none touch QGCTileCacheDatabase instance state.
namespace QGCTileDatabaseSchema {
bool createSchema(QSqlDatabase db, bool createDefault = true);
bool dropSchemaTables(QSqlDatabase db);
bool migrateSchema(QSqlDatabase db, int fromVersion);

// Brings an open connection's schema to the current version: migrates in
// place when possible, otherwise drops the tables and reports the reset via
// *didReset (the caller must then invalidate any cached default-set id).
// Returns false only on hard failure.
bool checkSchemaVersion(QSqlDatabase db, bool* didReset = nullptr);
}  // namespace QGCTileDatabaseSchema
