# Offline map cache internals

QGroundControl stores downloaded map tiles in an SQLite database located under the platform cache directory (for example `~/.cache/QGroundControl/QGCMapCache` on Linux). The file is named `qgcMapCache.db` and is created automatically the first time the QtLocation plugin runs.

## Schema version

The cache schema is versioned through `PRAGMA user_version`. QGroundControl v4.4 introduced schema version **2** (`kCurrentSchemaVersion` in `QGCTileCacheWorker`). On startup we verify the on-disk version and automatically rebuild the cache if it is missing or outdated. If you change the schema, update `kCurrentSchemaVersion`, describe the migration path in this document, and add a release note so that integrators know the cache will be rebuilt.

## Migration behaviour

* If the schema is missing or corrupted, `QGCTileCacheWorker::_verifyDatabaseVersion()` deletes the database and creates a fresh one.
* When importing a cache file, `_checkDatabaseVersion()` ensures the version matches `kCurrentSchemaVersion` and surfaces a user-facing error if it does not.
* Default tile sets are recreated on demand and always use map ID `UrlFactory::defaultSetMapId()`.

## Disabling caching

Two separate switches control caching:

* `setCachingPaused(true/false)` is used internally to stop saving tiles while maintenance tasks such as deletes and resets are in progress. Downloads are paused and resumed automatically.
* `MapsSettings.disableDefaultCache` exposes a user-facing “Default Cache” toggle. When disabled, tiles fetched during normal browsing are not persisted, but manually created offline sets continue to work.

When adding new features that manipulate the cache, prefer to go through `QGCMapEngineManager` so that pause/resume, download bookkeeping, and schema checks remain in sync.
