#include "GeoPackageHelper.h"
#include "GeoFileIO.h"
#include "GeoFormatRegistry.h"
#include "GeoUtilities.h"
#include "WKBHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QUuid>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

QGC_LOGGING_CATEGORY(GeoPackageHelperLog, "Utilities.Geo.GeoPackageHelper")

namespace GeoPackageHelper
{

namespace {

QString uniqueConnectionName()
{
    return QStringLiteral("gpkg_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

bool openDatabase(const QString &filePath, QSqlDatabase &db, QString &errorString)
{
    QString connectionName = uniqueConnectionName();
    db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(filePath);

    if (!db.open()) {
        errorString = QObject::tr("Cannot open database: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    return true;
}

void closeDatabase(QSqlDatabase &db)
{
    QString connectionName = db.connectionName();
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool createBaseTables(QSqlDatabase &db, QString &errorString)
{
    QSqlQuery query(db);

    // Set GeoPackage application ID
    if (!query.exec(QStringLiteral("PRAGMA application_id = 0x47504B47"))) { // "GPKG"
        errorString = QObject::tr("Failed to set application ID: %1").arg(query.lastError().text());
        return false;
    }

    // Set user version (1.2.1 = 10201)
    if (!query.exec(QStringLiteral("PRAGMA user_version = 10201"))) {
        errorString = QObject::tr("Failed to set user version: %1").arg(query.lastError().text());
        return false;
    }

    // Create gpkg_spatial_ref_sys
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS gpkg_spatial_ref_sys ("
            "srs_name TEXT NOT NULL, "
            "srs_id INTEGER PRIMARY KEY, "
            "organization TEXT NOT NULL, "
            "organization_coordsys_id INTEGER NOT NULL, "
            "definition TEXT NOT NULL, "
            "description TEXT)"))) {
        errorString = QObject::tr("Failed to create gpkg_spatial_ref_sys: %1").arg(query.lastError().text());
        return false;
    }

    // Insert WGS84
    if (!query.exec(QStringLiteral(
            "INSERT OR IGNORE INTO gpkg_spatial_ref_sys VALUES ("
            "'WGS 84 geodetic', 4326, 'EPSG', 4326, "
            "'GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],"
            "PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]', "
            "'WGS 84')"))) {
        errorString = QObject::tr("Failed to insert WGS84 SRS: %1").arg(query.lastError().text());
        return false;
    }

    // Create gpkg_contents
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS gpkg_contents ("
            "table_name TEXT PRIMARY KEY, "
            "data_type TEXT NOT NULL, "
            "identifier TEXT, "
            "description TEXT DEFAULT '', "
            "last_change TEXT DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')), "
            "min_x DOUBLE, min_y DOUBLE, max_x DOUBLE, max_y DOUBLE, "
            "srs_id INTEGER REFERENCES gpkg_spatial_ref_sys(srs_id))"))) {
        errorString = QObject::tr("Failed to create gpkg_contents: %1").arg(query.lastError().text());
        return false;
    }

    // Create gpkg_geometry_columns
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS gpkg_geometry_columns ("
            "table_name TEXT NOT NULL, "
            "column_name TEXT NOT NULL, "
            "geometry_type_name TEXT NOT NULL, "
            "srs_id INTEGER NOT NULL, "
            "z TINYINT NOT NULL, "
            "m TINYINT NOT NULL, "
            "CONSTRAINT pk_geom_cols PRIMARY KEY (table_name, column_name), "
            "CONSTRAINT fk_gc_tn FOREIGN KEY (table_name) REFERENCES gpkg_contents(table_name), "
            "CONSTRAINT fk_gc_srs FOREIGN KEY (srs_id) REFERENCES gpkg_spatial_ref_sys(srs_id))"))) {
        errorString = QObject::tr("Failed to create gpkg_geometry_columns: %1").arg(query.lastError().text());
        return false;
    }

    return true;
}

bool createFeatureTable(QSqlDatabase &db, const QString &tableName,
                        const QString &geometryType, QString &errorString)
{
    QSqlQuery query(db);

    // Create feature table
    QString sql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS %1 ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "geom BLOB, "
        "name TEXT)").arg(tableName);

    if (!query.exec(sql)) {
        errorString = QObject::tr("Failed to create feature table: %1").arg(query.lastError().text());
        return false;
    }

    // Register in gpkg_contents
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO gpkg_contents (table_name, data_type, identifier, srs_id) "
        "VALUES (?, 'features', ?, 4326)"));
    query.addBindValue(tableName);
    query.addBindValue(tableName);

    if (!query.exec()) {
        errorString = QObject::tr("Failed to register in gpkg_contents: %1").arg(query.lastError().text());
        return false;
    }

    // Register in gpkg_geometry_columns
    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO gpkg_geometry_columns "
        "(table_name, column_name, geometry_type_name, srs_id, z, m) "
        "VALUES (?, 'geom', ?, 4326, 0, 0)"));
    query.addBindValue(tableName);
    query.addBindValue(geometryType);

    if (!query.exec()) {
        errorString = QObject::tr("Failed to register geometry column: %1").arg(query.lastError().text());
        return false;
    }

    return true;
}

} // anonymous namespace

bool isValidGeoPackage(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        return false;
    }

    QString appId = getApplicationId(filePath);
    return appId == QStringLiteral("GPKG");
}

QString getApplicationId(const QString &filePath)
{
    QString errorString;
    QSqlDatabase db;

    if (!openDatabase(filePath, db, errorString)) {
        return QString();
    }

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA application_id"))) {
        closeDatabase(db);
        return QString();
    }

    QString result;
    if (query.next()) {
        quint32 appId = query.value(0).toUInt();
        // Convert to string: 0x47504B47 = "GPKG"
        char chars[5];
        chars[0] = (appId >> 24) & 0xFF;
        chars[1] = (appId >> 16) & 0xFF;
        chars[2] = (appId >> 8) & 0xFF;
        chars[3] = appId & 0xFF;
        chars[4] = 0;
        result = QString::fromLatin1(chars);
    }

    closeDatabase(db);
    return result;
}

int getVersion(const QString &filePath)
{
    QString errorString;
    QSqlDatabase db;

    if (!openDatabase(filePath, db, errorString)) {
        return 0;
    }

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA user_version"))) {
        closeDatabase(db);
        return 0;
    }

    int version = 0;
    if (query.next()) {
        version = query.value(0).toInt();
    }

    closeDatabase(db);
    return version;
}

bool listTables(const QString &filePath, QList<TableInfo> &tables, QString &errorString)
{
    QSqlDatabase db;
    if (!openDatabase(filePath, db, errorString)) {
        return false;
    }

    tables.clear();

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "SELECT c.table_name, g.column_name, g.geometry_type_name, g.srs_id "
            "FROM gpkg_contents c "
            "JOIN gpkg_geometry_columns g ON c.table_name = g.table_name "
            "WHERE c.data_type = 'features'"))) {
        errorString = QObject::tr("Failed to list tables: %1").arg(query.lastError().text());
        closeDatabase(db);
        return false;
    }

    while (query.next()) {
        TableInfo info;
        info.tableName = query.value(0).toString();
        info.geometryColumn = query.value(1).toString();
        info.geometryType = query.value(2).toString();
        info.srid = query.value(3).toInt();

        // Get feature count
        QSqlQuery countQuery(db);
        if (countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(info.tableName))) {
            if (countQuery.next()) {
                info.featureCount = countQuery.value(0).toInt();
            }
        }

        tables.append(info);
    }

    closeDatabase(db);
    return true;
}

LoadResult loadAllFeatures(const QString &filePath)
{
    LoadResult result;

    QList<TableInfo> tables;
    if (!listTables(filePath, tables, result.errorString)) {
        return result;
    }

    for (const TableInfo &table : tables) {
        LoadResult tableResult = loadTable(filePath, table.tableName);
        if (!tableResult.success) {
            result.errorString = tableResult.errorString;
            return result;
        }

        result.points.append(tableResult.points);
        result.polylines.append(tableResult.polylines);
        result.polygons.append(tableResult.polygons);
    }

    result.success = true;
    return result;
}

LoadResult loadTable(const QString &filePath, const QString &tableName)
{
    LoadResult result;

    QSqlDatabase db;
    if (!openDatabase(filePath, db, result.errorString)) {
        return result;
    }

    // Get geometry column name
    QSqlQuery metaQuery(db);
    metaQuery.prepare(QStringLiteral(
        "SELECT column_name FROM gpkg_geometry_columns WHERE table_name = ?"));
    metaQuery.addBindValue(tableName);

    if (!metaQuery.exec() || !metaQuery.next()) {
        result.errorString = QObject::tr("Cannot find geometry column for table: %1").arg(tableName);
        closeDatabase(db);
        return result;
    }

    QString geomColumn = metaQuery.value(0).toString();

    // Load geometries
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT %1 FROM %2").arg(geomColumn, tableName))) {
        result.errorString = QObject::tr("Failed to load features: %1").arg(query.lastError().text());
        closeDatabase(db);
        return result;
    }

    while (query.next()) {
        QByteArray gpbData = query.value(0).toByteArray();
        if (gpbData.isEmpty()) {
            continue;
        }

        // Parse GeoPackage binary header
        int srid, wkbOffset;
        QString parseError;

        if (!WKBHelper::parseGeoPackageHeader(gpbData, srid, wkbOffset, parseError)) {
            qCWarning(GeoPackageHelperLog) << "Failed to parse GPB header:" << parseError;
            continue;
        }

        // Extract WKB and parse geometry
        QByteArray wkb = gpbData.mid(wkbOffset);
        QList<QGeoCoordinate> points;
        QList<QList<QGeoCoordinate>> polylines;
        QList<QList<QGeoCoordinate>> polygons;

        if (!WKBHelper::parseGeometry(wkb, points, polylines, polygons, parseError)) {
            qCWarning(GeoPackageHelperLog) << "Failed to parse geometry:" << parseError;
            continue;
        }

        result.points.append(points);
        result.polylines.append(polylines);
        result.polygons.append(polygons);
    }

    closeDatabase(db);
    result.success = true;
    return result;
}

bool loadPoints(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString)
{
    LoadResult result = loadAllFeatures(filePath);
    if (!result.success) {
        errorString = result.errorString;
        return false;
    }

    points = result.points;
    return true;
}

bool loadPolylines(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString,
                   double filterMeters)
{
    LoadResult result = loadAllFeatures(filePath);
    if (!result.success) {
        errorString = result.errorString;
        return false;
    }

    polylines = result.polylines;

    // Apply vertex filtering if requested
    if (filterMeters > 0) {
        for (QList<QGeoCoordinate> &polyline : polylines) {
            GeoFormatRegistry::filterVertices(polyline, filterMeters, GeoUtilities::kMinPolylineVertices);
        }
    }

    return true;
}

bool loadPolygons(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString,
                  double filterMeters)
{
    LoadResult result = loadAllFeatures(filePath);
    if (!result.success) {
        errorString = result.errorString;
        return false;
    }

    polygons = result.polygons;

    // Apply vertex filtering if requested
    if (filterMeters > 0) {
        for (QList<QGeoCoordinate> &polygon : polygons) {
            GeoFormatRegistry::filterVertices(polygon, filterMeters, GeoUtilities::kMinPolygonVertices);
        }
    }

    return true;
}

bool loadPoint(const QString &filePath, QGeoCoordinate &point, QString &errorString)
{
    QList<QGeoCoordinate> points;
    if (!loadPoints(filePath, points, errorString)) {
        return false;
    }
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatNoEntitiesError(QStringLiteral("GeoPackage"), QObject::tr("points"));
        return false;
    }
    point = points.first();
    return true;
}

bool loadPolyline(const QString &filePath, QList<QGeoCoordinate> &polyline, QString &errorString,
                  double filterMeters)
{
    QList<QList<QGeoCoordinate>> polylines;
    if (!loadPolylines(filePath, polylines, errorString, filterMeters)) {
        return false;
    }
    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatNoEntitiesError(QStringLiteral("GeoPackage"), QObject::tr("polylines"));
        return false;
    }
    polyline = polylines.first();
    return true;
}

bool loadPolygon(const QString &filePath, QList<QGeoCoordinate> &polygon, QString &errorString,
                 double filterMeters)
{
    QList<QList<QGeoCoordinate>> polygons;
    if (!loadPolygons(filePath, polygons, errorString, filterMeters)) {
        return false;
    }
    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatNoEntitiesError(QStringLiteral("GeoPackage"), QObject::tr("polygons"));
        return false;
    }
    polygon = polygons.first();
    return true;
}

bool createGeoPackage(const QString &filePath, QString &errorString)
{
    // Remove existing file
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            errorString = GeoFileIO::formatSaveError(QStringLiteral("GeoPackage"), QObject::tr("Cannot remove existing file"));
            return false;
        }
    }

    QSqlDatabase db;
    if (!openDatabase(filePath, db, errorString)) {
        return false;
    }

    if (!createBaseTables(db, errorString)) {
        closeDatabase(db);
        return false;
    }

    closeDatabase(db);
    return true;
}

bool savePoints(const QString &filePath, const QList<QGeoCoordinate> &points,
                const QString &tableName, QString &errorString)
{
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("GeoPackage"), QObject::tr("No points to save"));
        return false;
    }

    // Create file if needed
    if (!QFile::exists(filePath)) {
        if (!createGeoPackage(filePath, errorString)) {
            return false;
        }
    }

    QSqlDatabase db;
    if (!openDatabase(filePath, db, errorString)) {
        return false;
    }

    if (!createFeatureTable(db, tableName, QStringLiteral("POINT"), errorString)) {
        closeDatabase(db);
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO %1 (geom, name) VALUES (?, ?)").arg(tableName));

    int index = 0;
    for (const QGeoCoordinate &point : points) {
        QByteArray wkb = WKBHelper::toWKBPoint(point, !std::isnan(point.altitude()));
        QByteArray gpb = WKBHelper::toGeoPackageBinary(wkb, 4326);

        query.addBindValue(gpb);
        query.addBindValue(QStringLiteral("Point_%1").arg(++index));

        if (!query.exec()) {
            errorString = GeoFileIO::formatSaveError(QStringLiteral("GeoPackage"),
                QObject::tr("Failed to insert point: %1").arg(query.lastError().text()));
            closeDatabase(db);
            return false;
        }
    }

    closeDatabase(db);
    return true;
}

bool savePolylines(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines,
                   const QString &tableName, QString &errorString)
{
    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("GeoPackage"), QObject::tr("No polylines to save"));
        return false;
    }

    if (!QFile::exists(filePath)) {
        if (!createGeoPackage(filePath, errorString)) {
            return false;
        }
    }

    QSqlDatabase db;
    if (!openDatabase(filePath, db, errorString)) {
        return false;
    }

    if (!createFeatureTable(db, tableName, QStringLiteral("LINESTRING"), errorString)) {
        closeDatabase(db);
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO %1 (geom, name) VALUES (?, ?)").arg(tableName));

    int index = 0;
    for (const QList<QGeoCoordinate> &coords : polylines) {
        bool hasZ = coords.count() > 0 && !std::isnan(coords.first().altitude());
        QByteArray wkb = WKBHelper::toWKBLineString(coords, hasZ);
        QByteArray gpb = WKBHelper::toGeoPackageBinary(wkb, 4326);

        query.addBindValue(gpb);
        query.addBindValue(QStringLiteral("Line_%1").arg(++index));

        if (!query.exec()) {
            errorString = GeoFileIO::formatSaveError(QStringLiteral("GeoPackage"),
                QObject::tr("Failed to insert polyline: %1").arg(query.lastError().text()));
            closeDatabase(db);
            return false;
        }
    }

    closeDatabase(db);
    return true;
}

bool savePolygons(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons,
                  const QString &tableName, QString &errorString)
{
    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("GeoPackage"), QObject::tr("No polygons to save"));
        return false;
    }

    if (!QFile::exists(filePath)) {
        if (!createGeoPackage(filePath, errorString)) {
            return false;
        }
    }

    QSqlDatabase db;
    if (!openDatabase(filePath, db, errorString)) {
        return false;
    }

    if (!createFeatureTable(db, tableName, QStringLiteral("POLYGON"), errorString)) {
        closeDatabase(db);
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO %1 (geom, name) VALUES (?, ?)").arg(tableName));

    int index = 0;
    for (const QList<QGeoCoordinate> &vertices : polygons) {
        bool hasZ = vertices.count() > 0 && !std::isnan(vertices.first().altitude());
        QByteArray wkb = WKBHelper::toWKBPolygon(vertices, hasZ);
        QByteArray gpb = WKBHelper::toGeoPackageBinary(wkb, 4326);

        query.addBindValue(gpb);
        query.addBindValue(QStringLiteral("Polygon_%1").arg(++index));

        if (!query.exec()) {
            errorString = GeoFileIO::formatSaveError(QStringLiteral("GeoPackage"),
                QObject::tr("Failed to insert polygon: %1").arg(query.lastError().text()));
            closeDatabase(db);
            return false;
        }
    }

    closeDatabase(db);
    return true;
}

} // namespace GeoPackageHelper
