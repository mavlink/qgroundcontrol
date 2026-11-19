#include "SHPFileHelper.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTextStream>

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "ShapeVertices.h"
#include "shapefil.h"

QGC_LOGGING_CATEGORY(SHPFileHelperLog, "Utilities.SHPFileHelper")

namespace {
    // QFile-based hooks for shapelib to support Qt Resource System (qrc:/) paths.
    // This allows loading shapefiles embedded in the application binary.

    SAFile qfileOpen(const char *filename, const char *access, void *pvUserData)
    {
        Q_UNUSED(pvUserData);

        if (!filename || !access) {
            qCWarning(SHPFileHelperLog) << "QFile open called with null filename or access mode";
            return nullptr;
        }

        // Only support read mode - shapefiles are read-only in QGC
        if (access[0] != 'r') {
            qCWarning(SHPFileHelperLog) << "QFile hooks only support read mode, requested:" << access;
            return nullptr;
        }

        auto *file = new QFile(QString::fromUtf8(filename));
        if (!file->open(QIODevice::ReadOnly)) {
            qCWarning(SHPFileHelperLog) << "Failed to open file:" << filename << file->errorString();
            delete file;
            return nullptr;
        }

        return reinterpret_cast<SAFile>(file);
    }

    SAOffset qfileRead(void *p, SAOffset size, SAOffset nmemb, SAFile file)
    {
        if (!file || !p || size == 0) {
            return 0;
        }
        auto *qfile = reinterpret_cast<QFile *>(file);
        const qint64 bytesRequested = static_cast<qint64>(size) * static_cast<qint64>(nmemb);
        const qint64 bytesRead = qfile->read(static_cast<char *>(p), bytesRequested);
        if (bytesRead < 0) {
            return 0;
        }
        return static_cast<SAOffset>(bytesRead / static_cast<qint64>(size));
    }

    SAOffset qfileWrite(const void *p, SAOffset size, SAOffset nmemb, SAFile file)
    {
        Q_UNUSED(p);
        Q_UNUSED(size);
        Q_UNUSED(nmemb);
        Q_UNUSED(file);
        qCWarning(SHPFileHelperLog) << "QFile write not supported - shapefiles are read-only in QGC";
        return 0;
    }

    SAOffset qfileSeek(SAFile file, SAOffset offset, int whence)
    {
        if (!file) {
            return -1;
        }
        auto *qfile = reinterpret_cast<QFile *>(file);
        qint64 newPos = 0;

        switch (whence) {
        case SEEK_SET:
            newPos = static_cast<qint64>(offset);
            break;
        case SEEK_CUR:
            newPos = qfile->pos() + static_cast<qint64>(offset);
            break;
        case SEEK_END:
            newPos = qfile->size() + static_cast<qint64>(offset);
            break;
        default:
            return -1;
        }

        if (newPos < 0) {
            return -1;
        }

        return qfile->seek(newPos) ? 0 : -1;
    }

    SAOffset qfileTell(SAFile file)
    {
        if (!file) {
            return 0;
        }
        auto *qfile = reinterpret_cast<QFile *>(file);
        return static_cast<SAOffset>(qfile->pos());
    }

    int qfileFlush(SAFile file)
    {
        Q_UNUSED(file);
        // No-op for read-only files
        return 0;
    }

    int qfileClose(SAFile file)
    {
        if (!file) {
            return 0;
        }
        auto *qfile = reinterpret_cast<QFile *>(file);
        qfile->close();
        delete qfile;
        return 0;
    }

    void setupQFileHooks(SAHooks *hooks)
    {
        SASetupDefaultHooks(hooks);
        hooks->FOpen = qfileOpen;
        hooks->FRead = qfileRead;
        hooks->FWrite = qfileWrite;
        hooks->FSeek = qfileSeek;
        hooks->FTell = qfileTell;
        hooks->FFlush = qfileFlush;
        hooks->FClose = qfileClose;
        hooks->Error = [](const char *message) {
            qCWarning(SHPFileHelperLog) << "SHP Error:" << message;
        };
    }
}

namespace SHPFileHelper
{
/// An open shapefile and what its .prj and its header say of it. Closes the file when it goes out of scope.
struct ShapeFile
{
    ShapeFile() = default;

    ~ShapeFile()
    {
        if (handle) {
            SHPClose(handle);
        }
    }
    Q_DISABLE_COPY_MOVE(ShapeFile)

    SHPHandle handle = nullptr;
    int utmZone = 0;  ///< 0 for a lat/lon shape
    bool utmSouthernHemisphere = false;
    int entityCount = 0;
    int shapeType = SHPT_NULL;
    bool hasAltitude = false;  ///< the shape type carries Z values; set by _openForType()
};

QString _loadError(const QString& detail);

/// Validates the specified SHP file is truly a SHP file and is in the format we understand.
///     @param utmZone[out] Zone for UTM shape, 0 for lat/lon shape
///     @param utmSouthernHemisphere[out] true/false for UTM hemisphere
/// @return true: Valid supported SHP file found, false: Invalid or unsupported file found
bool _validateSHPFiles(const QString& shpFile, int* utmZone, bool* utmSouthernHemisphere, QString& errorString);

/// Opens @a shpFile and reads its projection, entity count and shape type into @a shape.
/// @return false on failure, with @a errorString set
bool _open(const QString& shpFile, ShapeFile& shape, QString& errorString);

/// Opens @a shpFile, which must hold shapes of @a type or of @a zType, its variant with Z values.
/// @param wrongTypeError translated message for a file of another type, with %1 for the name of that type
/// @return false on failure, with @a errorString set
bool _openForType(const QString& shpFile, int type, int zType, const QString& wrongTypeError, ShapeFile& shape,
                  QString& errorString);

/// Calls @a visit with each entity of @a shape that can be read and with its index.
/// @param what the kind of entity, for the warning about one that cannot be read
template <typename Visit>
void _forEachEntity(const ShapeFile& shape, const char* what, Visit visit);

/// Reads the vertices of the first part of @a shpObject, the only part that is loaded, leaving out those that
/// _vertexCoordinate() rejects. With @a clockwise the part is a ring and is returned clockwise, as QGC polygons wind.
/// @param entityName,partName what the log calls the entity and its first part when there are more parts
QList<QGeoCoordinate> _firstPartVertices(const ShapeFile& shape, const SHPObject* shpObject, const char* entityName,
                                         const char* partName, bool clockwise);

/// Converts vertex @a index of @a shpObject to a coordinate, from UTM when the zone of @a shape is not 0.
/// @return false when the vertex is outside the range of the projection; a warning has been logged
bool _vertexCoordinate(const ShapeFile& shape, const SHPObject* shpObject, int index, QGeoCoordinate& coord);
}  // namespace SHPFileHelper

QString SHPFileHelper::_loadError(const QString& detail)
{
    return QCoreApplication::translate("SHPFileHelper", "SHP file load failed. %1").arg(detail);
}

bool SHPFileHelper::_validateSHPFiles(const QString &shpFile, int *utmZone, bool *utmSouthernHemisphere, QString &errorString)
{
    *utmZone = 0;
    errorString.clear();

    if (!shpFile.endsWith(QStringLiteral(".shp"), Qt::CaseInsensitive)) {
        errorString = _loadError(QCoreApplication::translate("SHP", "File is not a .shp file: %1").arg(shpFile));
        return false;
    }

    const QString prjFilename = shpFile.left(shpFile.length() - 4) + QStringLiteral(".prj");
    QFile prjFile(prjFilename);
    if (!prjFile.exists()) {
        errorString = _loadError(QCoreApplication::translate("SHP", "File not found: %1").arg(prjFilename));
        return false;
    }

    if (!prjFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = _loadError(
            QCoreApplication::translate("SHPFileHelper", "PRJ file open failed: %1").arg(prjFile.errorString()));
        return false;
    }

    QTextStream strm(&prjFile);
    const QString line = strm.readLine();
    if (line.startsWith(QStringLiteral("GEOGCS[\"GCS_WGS_1984\","))) {
        *utmZone = 0;
        *utmSouthernHemisphere = false;
    } else if (line.startsWith(QStringLiteral("PROJCS[\"WGS_1984_UTM_Zone_"))) {
        static const QRegularExpression regEx(QStringLiteral("^PROJCS\\[\"WGS_1984_UTM_Zone_(\\d{1,2})([NS])"));
        const QRegularExpressionMatch regExMatch = regEx.match(line);
        const QStringList rgCapture = regExMatch.capturedTexts();
        if (rgCapture.count() == 3) {
            const int zone = rgCapture[1].toInt();
            if ((zone >= 1) && (zone <= 60)) {
                *utmZone = zone;
                *utmSouthernHemisphere = (rgCapture[2] == QStringLiteral("S"));
            }
        }

        if (*utmZone == 0) {
            errorString = _loadError(QCoreApplication::translate(
                "SHP", "UTM projection is not in supported format. Must be PROJCS[\"WGS_1984_UTM_Zone_##N/S"));
            return false;
        }
    } else {
        // Extract projection name from WKT for error reporting
        // Format is either GEOGCS["name",... or PROJCS["name",...
        QString projectionName;
        static const QRegularExpression nameRegEx(QStringLiteral("^(?:GEOGCS|PROJCS)\\[\"([^\"]+)\""));
        const QRegularExpressionMatch nameMatch = nameRegEx.match(line);
        if (nameMatch.hasMatch()) {
            projectionName = nameMatch.captured(1);
        }

        if (!projectionName.isEmpty()) {
            errorString = _loadError(
                QCoreApplication::translate(
                    "SHP",
                    "Unsupported projection: %1. Supported projections are: WGS84 (GEOGCS[\"GCS_WGS_1984\"]) and UTM "
                    "(PROJCS[\"WGS_1984_UTM_Zone_##N/S\"]). Convert your shapefile to WGS84 using QGIS or ogr2ogr.")
                    .arg(projectionName));
        } else {
            errorString = _loadError(QCoreApplication::translate(
                "SHP",
                "Unable to parse projection from PRJ file. Supported projections are: WGS84 (GEOGCS[\"GCS_WGS_1984\"]) "
                "and UTM (PROJCS[\"WGS_1984_UTM_Zone_##N/S\"])."));
        }
        return false;
    }

    return true;
}

bool SHPFileHelper::_open(const QString& shpFile, ShapeFile& shape, QString& errorString)
{
    if (!_validateSHPFiles(shpFile, &shape.utmZone, &shape.utmSouthernHemisphere, errorString)) {
        return false;
    }

    // Use QFile-based hooks for Qt Resource System compatibility (qrc:/ paths)
    SAHooks sHooks{};
    setupQFileHooks(&sHooks);

    shape.handle = SHPOpenLL(shpFile.toUtf8().constData(), "rb", &sHooks);
    if (!shape.handle) {
        errorString = _loadError(QCoreApplication::translate("SHP", "SHPOpen failed."));
        return false;
    }

    SHPGetInfo(shape.handle, &shape.entityCount, &shape.shapeType, nullptr, nullptr);
    return true;
}

bool SHPFileHelper::_openForType(const QString& shpFile, int type, int zType, const QString& wrongTypeError,
                                 ShapeFile& shape, QString& errorString)
{
    if (!_open(shpFile, shape, errorString)) {
        return false;
    }

    if (shape.shapeType != type && shape.shapeType != zType) {
        errorString = _loadError(wrongTypeError.arg(SHPTypeName(shape.shapeType)));
        return false;
    }

    shape.hasAltitude = (shape.shapeType == zType);
    return true;
}

template <typename Visit>
void SHPFileHelper::_forEachEntity(const ShapeFile& shape, const char* what, Visit visit)
{
    for (int entityIdx = 0; entityIdx < shape.entityCount; entityIdx++) {
        SHPObject* shpObject = SHPReadObject(shape.handle, entityIdx);
        if (!shpObject) {
            qCWarning(SHPFileHelperLog) << "Failed to read" << what << "entity" << entityIdx;
            continue;
        }
        auto shpObjectCleanup = qScopeGuard([shpObject]() { SHPDestroyObject(shpObject); });

        visit(shpObject, entityIdx);
    }
}

QList<QGeoCoordinate> SHPFileHelper::_firstPartVertices(const ShapeFile& shape, const SHPObject* shpObject,
                                                        const char* entityName, const char* partName, bool clockwise)
{
    const int firstPartEnd = (shpObject->nParts > 1) ? shpObject->panPartStart[1] : shpObject->nVertices;
    if (shpObject->nParts > 1) {
        qCDebug(SHPFileHelperLog) << entityName << "entity" << shpObject->nShapeId << "has" << shpObject->nParts
                                  << "parts; using" << partName << "only (" << firstPartEnd << "vertices)";
    }

    QList<QGeoCoordinate> vertices;
    QList<int> kept;
    for (int i = 0; i < firstPartEnd; i++) {
        QGeoCoordinate coord;
        if (_vertexCoordinate(shape, shpObject, i, coord)) {
            vertices.append(coord);
            kept.append(i);
        }
    }

    if (clockwise) {
        // Twice the area of the ring that the kept vertices make, summed as SHPRewindObject() sums it, on the
        // numbers of the file: longitudes converted from UTM jump where a ring crosses the 180 degree meridian.
        double sum = 0;
        for (qsizetype i = 0; i < kept.count(); i++) {
            const int previous = kept[(i + kept.count() - 1) % kept.count()];
            const int next = kept[(i + 1) % kept.count()];
            sum += shpObject->padfX[kept[i]] * (shpObject->padfY[next] - shpObject->padfY[previous]);
        }
        if (sum > 0.0) {
            vertices = QList<QGeoCoordinate>(vertices.crbegin(), vertices.crend());
        }
    }
    return vertices;
}

bool SHPFileHelper::_vertexCoordinate(const ShapeFile& shape, const SHPObject* shpObject, int index,
                                      QGeoCoordinate& coord)
{
    if (shape.utmZone) {
        if (!QGCGeo::convertUTMToGeo(shpObject->padfX[index], shpObject->padfY[index], shape.utmZone,
                                     shape.utmSouthernHemisphere, coord)) {
            qCWarning(SHPFileHelperLog) << "UTM conversion failed for entity" << shpObject->nShapeId << "vertex"
                                        << index;
            return false;
        }
    } else {
        coord = QGeoCoordinate(shpObject->padfY[index], shpObject->padfX[index]);
        if (!coord.isValid()) {
            qCWarning(SHPFileHelperLog) << "Latitude or longitude out of range for entity" << shpObject->nShapeId
                                        << "vertex" << index << ":" << shpObject->padfY[index]
                                        << shpObject->padfX[index];
            return false;
        }
    }

    if (shape.hasAltitude && shpObject->padfZ) {
        coord.setAltitude(shpObject->padfZ[index]);
    }

    return true;
}

ShapeFileHelper::ShapeType SHPFileHelper::determineShapeType(const QString& shpFile, QString& errorString)
{
    using ShapeType = ShapeFileHelper::ShapeType;

    ShapeFile shape;
    if (!_open(shpFile, shape, errorString)) {
        return ShapeType::Error;
    }

    qCDebug(SHPFileHelperLog) << "SHPGetInfo" << shape.handle << shape.entityCount << shape.shapeType;
    if (shape.entityCount < 1) {
        errorString = _loadError(QCoreApplication::translate("SHP", "No entities found."));
    } else if (shape.shapeType == SHPT_POLYGON || shape.shapeType == SHPT_POLYGONZ) {
        return ShapeType::Polygon;
    } else if (shape.shapeType == SHPT_ARC || shape.shapeType == SHPT_ARCZ) {
        return ShapeType::Polyline;
    } else if (shape.shapeType == SHPT_POINT || shape.shapeType == SHPT_POINTZ) {
        return ShapeType::Point;
    } else {
        errorString = _loadError(QCoreApplication::translate("SHP", "No supported types found."));
    }

    return ShapeType::Error;
}

int SHPFileHelper::getEntityCount(const QString& shpFile, QString& errorString)
{
    ShapeFile shape;
    return _open(shpFile, shape, errorString) ? shape.entityCount : -1;
}

bool SHPFileHelper::loadPolygonsFromFile(const QString& shpFile, QList<QList<QGeoCoordinate>>& polygons,
                                         QString& errorString, double filterMeters)
{
    polygons.clear();

    ShapeFile shape;
    if (!_openForType(shpFile, SHPT_POLYGON, SHPT_POLYGONZ,
                      QCoreApplication::translate("SHPFileHelper", "File contains %1, expected Polygon."), shape,
                      errorString)) {
        return false;
    }

    _forEachEntity(shape, "polygon", [&](const SHPObject* shpObject, int entityIdx) {
        // For multi-part polygons (e.g., polygons with holes), we extract only the outer ring.
        // In shapefiles, the first part is conventionally the outer boundary, and subsequent
        // parts are holes (inner rings). For QGC's use cases (survey areas, geofences), the
        // outer boundary is what matters for mission planning.
        QList<QGeoCoordinate> vertices = _firstPartVertices(shape, shpObject, "Polygon", "outer ring", true);

        if (vertices.count() < 3) {
            qCWarning(SHPFileHelperLog) << "Skipping polygon entity" << entityIdx << "with less than 3 vertices";
            return;
        }

        // The repeated first vertex goes before filtering, so that it does not count towards the three vertices
        // the filter must leave.
        ShapeVertices::removeClosingVertex(vertices);
        ShapeVertices::filterNearby(vertices, filterMeters, 3);

        polygons.append(vertices);
    });

    if (polygons.isEmpty()) {
        errorString = _loadError(QCoreApplication::translate("SHP", "No valid polygons found."));
        return false;
    }

    return true;
}

bool SHPFileHelper::loadPolylinesFromFile(const QString &shpFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    polylines.clear();

    ShapeFile shape;
    if (!_openForType(shpFile, SHPT_ARC, SHPT_ARCZ,
                      QCoreApplication::translate("SHPFileHelper", "File contains %1, expected Arc."), shape,
                      errorString)) {
        return false;
    }

    _forEachEntity(shape, "polyline", [&](const SHPObject* shpObject, int entityIdx) {
        // For multi-part polylines (disconnected segments), we extract only the first part.
        // This maintains consistency with polygon handling and provides the primary path.
        // Each part in a multi-part polyline is typically a separate disconnected segment.
        QList<QGeoCoordinate> vertices = _firstPartVertices(shape, shpObject, "Polyline", "first part", false);

        if (vertices.count() < 2) {
            qCWarning(SHPFileHelperLog) << "Skipping polyline entity" << entityIdx << "with less than 2 vertices";
            return;
        }

        ShapeVertices::filterNearby(vertices, filterMeters, 2);

        polylines.append(vertices);
    });

    if (polylines.isEmpty()) {
        errorString = _loadError(QCoreApplication::translate("SHP", "No valid polylines found."));
        return false;
    }

    return true;
}

bool SHPFileHelper::loadPointsFromFile(const QString &shpFile, QList<QGeoCoordinate> &points, QString &errorString)
{
    points.clear();

    ShapeFile shape;
    if (!_openForType(shpFile, SHPT_POINT, SHPT_POINTZ,
                      QCoreApplication::translate("SHPFileHelper", "File contains %1, expected Point."), shape,
                      errorString)) {
        return false;
    }

    _forEachEntity(shape, "point", [&](const SHPObject* shpObject, int entityIdx) {
        // Point shapes have exactly one vertex per entity
        if (shpObject->nVertices != 1) {
            qCWarning(SHPFileHelperLog) << "Skipping point entity" << entityIdx << "with unexpected vertex count:" << shpObject->nVertices;
            return;
        }

        QGeoCoordinate coord;
        if (_vertexCoordinate(shape, shpObject, 0, coord)) {
            points.append(coord);
        }
    });

    if (points.isEmpty()) {
        errorString = _loadError(QCoreApplication::translate("SHP", "No valid points found."));
        return false;
    }

    return true;
}
