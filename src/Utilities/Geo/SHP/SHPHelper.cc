#include "SHPHelper.h"
#include "GeoFileIO.h"
#include "GeoUtilities.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <cmath>
#include <limits>
#include <memory>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "shapefil.h"

#include <proj.h>

QGC_LOGGING_CATEGORY(SHPHelperLog, "Utilities.Geo.SHPHelper")

namespace {
    constexpr const char *kFormatName = "SHP";

    QString _unsupportedProjectionError(const QString &detail)
    {
        return GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
                                          QObject::tr("Unsupported projection in PRJ file (%1).").arg(detail));
    }

    struct ShpCoordinateTransformer {
        enum class Mode {
            Wgs84Geographic,
            UTM,
            Proj
        };

        Mode mode = Mode::Wgs84Geographic;
        int utmZone = 0;
        bool utmSouthernHemisphere = false;

        PJ_CONTEXT *projContext = nullptr;
        PJ *projToWgs84 = nullptr;

        ~ShpCoordinateTransformer()
        {
            reset();
        }

        void reset()
        {
            if (projToWgs84) {
                proj_destroy(projToWgs84);
                projToWgs84 = nullptr;
            }
            if (projContext) {
                proj_context_destroy(projContext);
                projContext = nullptr;
            }
            mode = Mode::Wgs84Geographic;
            utmZone = 0;
            utmSouthernHemisphere = false;
        }

        bool initializeFromPrj(const QString &prjText, QString &errorString)
        {
            reset();
            errorString.clear();

            const QString prjWkt = prjText.trimmed();
            if (prjWkt.isEmpty()) {
                errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName), QObject::tr("PRJ file is empty."));
                return false;
            }

            if (prjWkt.startsWith(QStringLiteral("GEOGCS[\"GCS_WGS_1984\","))) {
                mode = Mode::Wgs84Geographic;
                return true;
            }

            if (prjWkt.startsWith(QStringLiteral("PROJCS[\"WGS_1984_UTM_Zone_"))) {
                static const QRegularExpression regEx(QStringLiteral("^PROJCS\\[\"WGS_1984_UTM_Zone_(\\d{1,2})([NS])"));
                const QRegularExpressionMatch regExMatch = regEx.match(prjWkt);
                if (regExMatch.hasMatch()) {
                    const int zone = regExMatch.captured(1).toInt();
                    if ((zone >= 1) && (zone <= 60)) {
                        mode = Mode::UTM;
                        utmZone = zone;
                        utmSouthernHemisphere = (regExMatch.captured(2) == QStringLiteral("S"));
                        return true;
                    }
                }

                errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
                                                         QT_TRANSLATE_NOOP("SHP", "UTM projection is not in supported format. Must be PROJCS[\"WGS_1984_UTM_Zone_##N/S"));
                return false;
            }

            projContext = proj_context_create();
            if (!projContext) {
                errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName), QObject::tr("Failed to initialize PROJ context."));
                return false;
            }

            const QByteArray sourceCrs = prjWkt.toUtf8();
            PJ *transform = proj_create_crs_to_crs(projContext, sourceCrs.constData(), "EPSG:4326", nullptr);
            if (!transform) {
                errorString = _unsupportedProjectionError(QStringLiteral("PROJ could not create transform"));
                return false;
            }

            projToWgs84 = proj_normalize_for_visualization(projContext, transform);
            proj_destroy(transform);
            if (!projToWgs84) {
                errorString = _unsupportedProjectionError(QStringLiteral("PROJ normalization failed"));
                return false;
            }

            mode = Mode::Proj;
            return true;
        }

        bool toGeo(double x, double y, QGeoCoordinate &coord) const
        {
            switch (mode) {
            case Mode::Wgs84Geographic:
                coord.setLatitude(y);
                coord.setLongitude(x);
                return true;
            case Mode::UTM:
                return QGCGeo::convertUTMToGeo(x, y, utmZone, utmSouthernHemisphere, coord);
            case Mode::Proj:
                if (!projToWgs84) {
                    return false;
                }

                const PJ_COORD input = proj_coord(x, y, 0, 0);
                const PJ_COORD output = proj_trans(projToWgs84, PJ_FWD, input);
                if (!std::isfinite(output.xy.x) || !std::isfinite(output.xy.y)) {
                    return false;
                }
                coord.setLongitude(output.xy.x);
                coord.setLatitude(output.xy.y);
                return true;
            }

            return false;
        }
    };

    // Create validation callback for SHP coordinate logging
    GeoUtilities::ValidationWarningCallback makeValidationCallback(int entityIdx)
    {
        return [entityIdx](int vertexIdx, const QString &type, double value) {
            qCWarning(SHPHelperLog) << "Entity" << entityIdx << "vertex" << vertexIdx
                                        << type << "out of range:" << value;
        };
    }

    QString _prjPathFromShpPath(const QString &shpFile)
    {
        const QFileInfo shpInfo(shpFile);
        return shpInfo.dir().filePath(shpInfo.completeBaseName() + QStringLiteral(".prj"));
    }

    QList<QGeoCoordinate> _extractPartVertices(const SHPObject *shpObject,
                                               int startIndex,
                                               int endIndex,
                                               const ShpCoordinateTransformer &transformer,
                                               bool includeAltitude,
                                               int entityIdx)
    {
        QList<QGeoCoordinate> vertices;
        vertices.reserve(qMax(0, endIndex - startIndex));

        for (int i = startIndex; i < endIndex; ++i) {
            QGeoCoordinate coord;
            if (!transformer.toGeo(shpObject->padfX[i], shpObject->padfY[i], coord)) {
                qCWarning(SHPHelperLog) << "Coordinate conversion failed for entity" << entityIdx << "vertex" << i;
                continue;
            }
            if (includeAltitude) {
                coord.setAltitude(shpObject->padfZ[i]);
            }
            vertices.append(coord);
        }

        return vertices;
    }

    bool _ensureValidShpHandle(SHPHandle shpHandle, QString &errorString)
    {
        if (shpHandle != nullptr) {
            return true;
        }

        if (errorString.isEmpty()) {
            errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
                                                     QT_TRANSLATE_NOOP("SHP", "SHPOpen failed."));
        }

        return false;
    }

    // QFile-based hooks for shapelib to support Qt Resource System (qrc:/) paths.
    // This allows loading shapefiles embedded in the application binary.

    SAFile qfileOpen(const char *filename, const char *access, void *pvUserData)
    {
        Q_UNUSED(pvUserData);

        if (!filename || !access) {
            qCWarning(SHPHelperLog) << "QFile open called with null filename or access mode";
            return nullptr;
        }

        // Only support read mode - shapefiles are read-only in QGC
        if (access[0] != 'r') {
            qCWarning(SHPHelperLog) << "QFile hooks only support read mode, requested:" << access;
            return nullptr;
        }

        // Use unique_ptr for RAII - release ownership only on success
        auto file = std::make_unique<QFile>(QString::fromUtf8(filename));
        if (!file->open(QIODevice::ReadOnly)) {
            qCWarning(SHPHelperLog) << "Failed to open file:" << filename << file->errorString();
            return nullptr;  // unique_ptr auto-deletes on failure
        }

        // Transfer ownership to shapelib (will be deleted in qfileClose)
        return reinterpret_cast<SAFile>(file.release());
    }

    SAOffset qfileRead(void *p, SAOffset size, SAOffset nmemb, SAFile file)
    {
        if (!file || !p || size <= 0 || nmemb <= 0) {
            return 0;
        }

        constexpr qint64 kMaxQint64 = std::numeric_limits<qint64>::max();
        if (size > (kMaxQint64 / nmemb)) {
            qCWarning(SHPHelperLog) << "qfileRead request overflow, size:" << size << "nmemb:" << nmemb;
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
        qCWarning(SHPHelperLog) << "QFile write not supported - shapefiles are read-only in QGC";
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
            qCWarning(SHPHelperLog) << "SHP Error:" << message;
        };
    }
}

namespace SHPHelper
{
    /// Validates the specified SHP file and initializes a coordinate transformer.
    /// @return true: Valid supported SHP file found, false: Invalid or unsupported file found
    bool _validateSHPFiles(const QString &shpFile, ShpCoordinateTransformer &transformer, QString &errorString);

    SHPHandle _loadShape(const QString &shpFile, ShpCoordinateTransformer &transformer, QString &errorString);
}

bool SHPHelper::_validateSHPFiles(const QString &shpFile, ShpCoordinateTransformer &transformer, QString &errorString)
{
    transformer.reset();
    errorString.clear();

    if (!shpFile.endsWith(QStringLiteral(".shp"), Qt::CaseInsensitive)) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QString(QT_TRANSLATE_NOOP("SHP", "File is not a .shp file: %1")).arg(shpFile));
        return false;
    }

    const QString prjFilename = _prjPathFromShpPath(shpFile);
    const auto prjResult = GeoFileIO::loadText(prjFilename, QString::fromLatin1(kFormatName));
    if (!prjResult.success) {
        errorString = prjResult.error;
        return false;
    }

    return transformer.initializeFromPrj(prjResult.content, errorString);
}

SHPHandle SHPHelper::_loadShape(const QString &shpFile, ShpCoordinateTransformer &transformer, QString &errorString)
{
    errorString.clear();

    if (!_validateSHPFiles(shpFile, transformer, errorString)) {
        return nullptr;
    }

    // Use QFile-based hooks for Qt Resource System compatibility (qrc:/ paths)
    SAHooks sHooks{};
    setupQFileHooks(&sHooks);

    SHPHandle shpHandle = SHPOpenLL(shpFile.toUtf8().constData(), "rb", &sHooks);
    if (shpHandle == nullptr) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "SHPOpen failed."));
    }

    return shpHandle;
}

GeoFormatRegistry::ShapeType SHPHelper::determineShapeType(const QString &shpFile, QString &errorString)
{
    using ShapeType = GeoFormatRegistry::ShapeType;

    ShapeType shapeType = ShapeType::Error;

    errorString.clear();

    ShpCoordinateTransformer transformer;
    SHPHandle shpHandle = SHPHelper::_loadShape(shpFile, transformer, errorString);
    if (_ensureValidShpHandle(shpHandle, errorString)) {

        int cEntities, type;
        SHPGetInfo(shpHandle, &cEntities, &type, nullptr, nullptr);
        qCDebug(SHPHelperLog) << "SHPGetInfo" << shpHandle << cEntities << type;
        if (cEntities < 1) {
            errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No entities found."));
        } else if (type == SHPT_POLYGON || type == SHPT_POLYGONZ) {
            shapeType = ShapeType::Polygon;
        } else if (type == SHPT_ARC || type == SHPT_ARCZ) {
            shapeType = ShapeType::Polyline;
        } else if (type == SHPT_POINT || type == SHPT_POINTZ ||
                   type == SHPT_MULTIPOINT || type == SHPT_MULTIPOINTZ) {
            shapeType = ShapeType::Point;
        } else {
            errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No supported types found."));
        }
    }

    if (shpHandle != nullptr) {
        SHPClose(shpHandle);
    }

    return shapeType;
}

int SHPHelper::getEntityCount(const QString &shpFile, QString &errorString)
{
    errorString.clear();

    ShpCoordinateTransformer transformer;
    SHPHandle shpHandle = SHPHelper::_loadShape(shpFile, transformer, errorString);
    if (!_ensureValidShpHandle(shpHandle, errorString)) {
        return 0;
    }

    int cEntities, type;
    SHPGetInfo(shpHandle, &cEntities, &type, nullptr, nullptr);
    SHPClose(shpHandle);

    return cEntities;
}

bool SHPHelper::loadPolygonFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polygons;
    if (!loadPolygonsFromFile(shpFile, polygons, errorString, filterMeters)) {
        return false;
    }
    vertices = polygons.first();
    return true;
}

bool SHPHelper::loadPolylineFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polylines;
    if (!loadPolylinesFromFile(shpFile, polylines, errorString, filterMeters)) {
        return false;
    }
    vertices = polylines.first();
    return true;
}

bool SHPHelper::loadPolygonsFromFile(const QString &shpFile, QList<QList<QGeoCoordinate>> &polygons, QString &errorString, double filterMeters)
{
    ShpCoordinateTransformer transformer;
    SHPHandle shpHandle = nullptr;

    errorString.clear();
    polygons.clear();

    auto cleanup = qScopeGuard([&]() {
        if (shpHandle != nullptr) SHPClose(shpHandle);
    });

    shpHandle = SHPHelper::_loadShape(shpFile, transformer, errorString);
    if (!_ensureValidShpHandle(shpHandle, errorString)) {
        return false;
    }
    
    int cEntities, shapeType;
    SHPGetInfo(shpHandle, &cEntities, &shapeType, nullptr, nullptr);
    if (shapeType != SHPT_POLYGON && shapeType != SHPT_POLYGONZ) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("File contains %1, expected Polygon.").arg(SHPTypeName(shapeType)));
        return false;
    }

    const bool hasAltitude = (shapeType == SHPT_POLYGONZ);

    for (int entityIdx = 0; entityIdx < cEntities; entityIdx++) {
        SHPObject *shpObject = SHPReadObject(shpHandle, entityIdx);
        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to read polygon entity" << entityIdx;
            continue;
        }
        auto objectCleanup = qScopeGuard([shpObject]() { SHPDestroyObject(shpObject); });

        // Ensure clockwise winding for outer rings (QGC requirement)
        SHPRewindObject(shpHandle, shpObject);

        // Note: This function ignores holes. Use loadPolygonsWithHolesFromFile() to preserve hole information.
        const int firstPartEnd = (shpObject->nParts > 1) ? shpObject->panPartStart[1] : shpObject->nVertices;
        if (shpObject->nParts > 1) {
            qCDebug(SHPHelperLog) << "Polygon entity" << entityIdx << "has" << shpObject->nParts
                                      << "parts; using outer ring only (" << firstPartEnd << "vertices)";
        }

        const bool entityHasAltitude = hasAltitude && shpObject->padfZ;
        QList<QGeoCoordinate> vertices = _extractPartVertices(shpObject, 0, firstPartEnd, transformer, entityHasAltitude, entityIdx);

        // SHP format requires closed rings (first == last), but QGC uses open polygons
        GeoUtilities::removeClosingVertex(vertices);

        if (vertices.count() < GeoUtilities::kMinPolygonVertices) {
            qCWarning(SHPHelperLog) << "Skipping polygon entity" << entityIdx << "with less than 3 vertices";
            continue;
        }

        // Validate and normalize coordinates
        GeoUtilities::validateAndNormalizeCoordinates(vertices, makeValidationCallback(entityIdx));

        // Filter nearby vertices if enabled
        GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolygonVertices);

        polygons.append(vertices);
    }

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No valid polygons found."));
        return false;
    }

    return true;
}

bool SHPHelper::loadPolygonWithHolesFromFile(const QString &shpFile, QGeoPolygon &polygon, QString &errorString)
{
    QList<QGeoPolygon> polygons;
    if (!loadPolygonsWithHolesFromFile(shpFile, polygons, errorString)) {
        return false;
    }
    polygon = polygons.first();
    return true;
}

bool SHPHelper::loadPolygonsWithHolesFromFile(const QString &shpFile, QList<QGeoPolygon> &polygons, QString &errorString)
{
    ShpCoordinateTransformer transformer;
    SHPHandle shpHandle = nullptr;

    errorString.clear();
    polygons.clear();

    auto cleanup = qScopeGuard([&]() {
        if (shpHandle != nullptr) SHPClose(shpHandle);
    });

    shpHandle = SHPHelper::_loadShape(shpFile, transformer, errorString);
    if (!_ensureValidShpHandle(shpHandle, errorString)) {
        return false;
    }
    
    int cEntities, shapeType;
    SHPGetInfo(shpHandle, &cEntities, &shapeType, nullptr, nullptr);
    if (shapeType != SHPT_POLYGON && shapeType != SHPT_POLYGONZ) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("File contains %1, expected Polygon.").arg(SHPTypeName(shapeType)));
        return false;
    }

    const bool hasAltitude = (shapeType == SHPT_POLYGONZ);

    for (int entityIdx = 0; entityIdx < cEntities; entityIdx++) {
        SHPObject *shpObject = SHPReadObject(shpHandle, entityIdx);
        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to read polygon entity" << entityIdx;
            continue;
        }
        auto objectCleanup = qScopeGuard([shpObject]() { SHPDestroyObject(shpObject); });

        // Ensure clockwise winding for outer rings
        SHPRewindObject(shpHandle, shpObject);

        // Multi-part polygons: part 0 is outer ring, parts 1..N are holes
        if (shpObject->nParts < 1) {
            qCWarning(SHPHelperLog) << "Polygon entity" << entityIdx << "has no parts, skipping";
            continue;
        }

        const bool entityHasAltitude = hasAltitude && shpObject->padfZ;

        // Parse outer ring (part 0)
        const int outerStart = shpObject->panPartStart[0];
        const int outerEnd = (shpObject->nParts > 1) ? shpObject->panPartStart[1] : shpObject->nVertices;

        QList<QGeoCoordinate> outerRing = _extractPartVertices(shpObject, outerStart, outerEnd, transformer, entityHasAltitude, entityIdx);

        // Remove closing vertex (SHP rings are closed, QGC uses open)
        GeoUtilities::removeClosingVertex(outerRing);

        if (outerRing.count() < GeoUtilities::kMinPolygonVertices) {
            qCWarning(SHPHelperLog) << "Polygon entity" << entityIdx << "outer ring has less than 3 vertices, skipping";
            continue;
        }

        // Validate and normalize outer ring coordinates
        GeoUtilities::validateAndNormalizeCoordinates(outerRing, makeValidationCallback(entityIdx));

        QGeoPolygon geoPolygon(outerRing);

        // Parse inner rings (holes) - parts 1..N
        for (int partIdx = 1; partIdx < shpObject->nParts; partIdx++) {
            const int holeStart = shpObject->panPartStart[partIdx];
            const int holeEnd = (partIdx + 1 < shpObject->nParts) ? shpObject->panPartStart[partIdx + 1] : shpObject->nVertices;

            QList<QGeoCoordinate> holeRing = _extractPartVertices(shpObject, holeStart, holeEnd, transformer, entityHasAltitude, entityIdx);

            // Remove closing vertex
            GeoUtilities::removeClosingVertex(holeRing);

            if (holeRing.count() >= GeoUtilities::kMinPolygonVertices) {
                // Validate and normalize hole ring coordinates
                GeoUtilities::validateAndNormalizeCoordinates(holeRing, makeValidationCallback(entityIdx));
                geoPolygon.addHole(holeRing);
            }
        }

        polygons.append(geoPolygon);
    }

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No valid polygons found."));
        return false;
    }

    return true;
}

bool SHPHelper::loadPolylinesFromFile(const QString &shpFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    ShpCoordinateTransformer transformer;
    SHPHandle shpHandle = nullptr;

    errorString.clear();
    polylines.clear();

    auto cleanup = qScopeGuard([&]() {
        if (shpHandle != nullptr) SHPClose(shpHandle);
    });

    shpHandle = SHPHelper::_loadShape(shpFile, transformer, errorString);
    if (!_ensureValidShpHandle(shpHandle, errorString)) {
        return false;
    }
    
    int cEntities, shapeType;
    SHPGetInfo(shpHandle, &cEntities, &shapeType, nullptr, nullptr);
    if (shapeType != SHPT_ARC && shapeType != SHPT_ARCZ) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("File contains %1, expected Arc.").arg(SHPTypeName(shapeType)));
        return false;
    }

    const bool hasAltitude = (shapeType == SHPT_ARCZ);

    for (int entityIdx = 0; entityIdx < cEntities; entityIdx++) {
        SHPObject *shpObject = SHPReadObject(shpHandle, entityIdx);
        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to read polyline entity" << entityIdx;
            continue;
        }
        auto objectCleanup = qScopeGuard([shpObject]() { SHPDestroyObject(shpObject); });

        // For multi-part polylines (disconnected segments), we extract only the first part.
        // This maintains consistency with polygon handling and provides the primary path.
        // Each part in a multi-part polyline is typically a separate disconnected segment.
        const int firstPartEnd = (shpObject->nParts > 1) ? shpObject->panPartStart[1] : shpObject->nVertices;
        if (shpObject->nParts > 1) {
            qCDebug(SHPHelperLog) << "Polyline entity" << entityIdx << "has" << shpObject->nParts
                                      << "parts; using first part only (" << firstPartEnd << "vertices)";
        }

        const bool entityHasAltitude = hasAltitude && shpObject->padfZ;
        QList<QGeoCoordinate> vertices = _extractPartVertices(shpObject, 0, firstPartEnd, transformer, entityHasAltitude, entityIdx);

        if (vertices.count() < GeoUtilities::kMinPolylineVertices) {
            qCWarning(SHPHelperLog) << "Skipping polyline entity" << entityIdx << "with less than 2 vertices";
            continue;
        }

        // Validate and normalize coordinates
        GeoUtilities::validateAndNormalizeCoordinates(vertices, makeValidationCallback(entityIdx));

        // Filter nearby vertices if enabled
        GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolylineVertices);

        polylines.append(vertices);
    }

    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No valid polylines found."));
        return false;
    }

    return true;
}

bool SHPHelper::loadPointFromFile(const QString &shpFile, QGeoCoordinate &point, QString &errorString)
{
    QList<QGeoCoordinate> points;
    if (!loadPointsFromFile(shpFile, points, errorString)) {
        return false;
    }
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName), QT_TRANSLATE_NOOP("SHP", "No points found."));
        return false;
    }
    point = points.first();
    return true;
}

bool SHPHelper::loadPointsFromFile(const QString &shpFile, QList<QGeoCoordinate> &points, QString &errorString)
{
    ShpCoordinateTransformer transformer;
    SHPHandle shpHandle = nullptr;

    errorString.clear();
    points.clear();

    auto cleanup = qScopeGuard([&]() {
        if (shpHandle != nullptr) SHPClose(shpHandle);
    });

    shpHandle = SHPHelper::_loadShape(shpFile, transformer, errorString);
    if (!_ensureValidShpHandle(shpHandle, errorString)) {
        return false;
    }
    
    int cEntities, shapeType;
    SHPGetInfo(shpHandle, &cEntities, &shapeType, nullptr, nullptr);
    const bool isPointType = (shapeType == SHPT_POINT || shapeType == SHPT_POINTZ);
    const bool isMultiPointType = (shapeType == SHPT_MULTIPOINT || shapeType == SHPT_MULTIPOINTZ);
    if (!isPointType && !isMultiPointType) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("File contains %1, expected Point.").arg(SHPTypeName(shapeType)));
        return false;
    }

    const bool hasAltitude = (shapeType == SHPT_POINTZ || shapeType == SHPT_MULTIPOINTZ);

    for (int entityIdx = 0; entityIdx < cEntities; entityIdx++) {
        SHPObject *shpObject = SHPReadObject(shpHandle, entityIdx);
        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to read point entity" << entityIdx;
            continue;
        }
        auto objectCleanup = qScopeGuard([shpObject]() { SHPDestroyObject(shpObject); });

        const auto appendVertex = [&](int vertexIdx) {
            QGeoCoordinate coord;
            if (!transformer.toGeo(shpObject->padfX[vertexIdx], shpObject->padfY[vertexIdx], coord)) {
                qCWarning(SHPHelperLog) << "Coordinate conversion failed for point entity" << entityIdx
                                        << "vertex" << vertexIdx;
                return;
            }

            if (hasAltitude && shpObject->padfZ) {
                coord.setAltitude(shpObject->padfZ[vertexIdx]);
            }

            // Validate and normalize coordinate
            if (!GeoUtilities::isValidCoordinate(coord)) {
                qCWarning(SHPHelperLog) << "Point entity" << entityIdx << "vertex" << vertexIdx
                                        << "coordinate out of range, normalizing";
                coord = GeoUtilities::normalizeCoordinate(coord);
            }
            if (!std::isnan(coord.altitude()) && !GeoUtilities::isValidAltitude(coord.altitude())) {
                qCWarning(SHPHelperLog) << "Point entity" << entityIdx << "vertex" << vertexIdx
                                        << "altitude out of expected range:" << coord.altitude();
            }

            points.append(coord);
        };

        if (isPointType) {
            // Point shapes have exactly one vertex per entity.
            if (shpObject->nVertices != 1) {
                qCWarning(SHPHelperLog) << "Skipping point entity" << entityIdx
                                        << "with unexpected vertex count:" << shpObject->nVertices;
                continue;
            }
            appendVertex(0);
            continue;
        }

        // MultiPoint entities may contain one or more vertices.
        if (shpObject->nVertices < 1) {
            qCWarning(SHPHelperLog) << "Skipping multipoint entity" << entityIdx << "with no vertices";
            continue;
        }

        for (int vertexIdx = 0; vertexIdx < shpObject->nVertices; ++vertexIdx) {
            appendVertex(vertexIdx);
        }
    }

    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No valid points found."));
        return false;
    }

    return true;
}

// ============================================================================
// Save functions
// ============================================================================

bool SHPHelper::savePolygonToFile(const QString &shpFile, const QList<QGeoCoordinate> &vertices, QString &errorString)
{
    return savePolygonsToFile(shpFile, {vertices}, errorString);
}

bool SHPHelper::savePolygonsToFile(const QString &shpFile, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString)
{
    errorString.clear();

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No polygons to save."));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validatePolygonListCoordinates(polygons, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        return false;
    }

    // Check if any coordinate has altitude - use PolygonZ if so
    const bool useZ = GeoUtilities::hasAnyAltitude(polygons);
    const int shapeType = useZ ? SHPT_POLYGONZ : SHPT_POLYGON;

    SHPHandle shpHandle = SHPCreate(shpFile.toUtf8().constData(), shapeType);
    if (shpHandle == nullptr) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "Unable to create SHP file."));
        return false;
    }

    auto cleanup = qScopeGuard([&]() {
        SHPClose(shpHandle);
    });

    for (const QList<QGeoCoordinate> &vertices : polygons) {
        if (vertices.count() < 3) {
            qCWarning(SHPHelperLog) << "Skipping polygon with less than 3 vertices";
            continue;
        }

        // Shapelib expects closed polygons (first == last), so add closing vertex if needed
        const bool needsClosing = (vertices.first() != vertices.last());
        const int nVertices = static_cast<int>(vertices.count()) + (needsClosing ? 1 : 0);

        QVector<double> padfX(nVertices);
        QVector<double> padfY(nVertices);
        QVector<double> padfZ(useZ ? nVertices : 0);

        for (int i = 0; i < vertices.count(); i++) {
            padfX[i] = vertices[i].longitude();
            padfY[i] = vertices[i].latitude();
            if (useZ) {
                padfZ[i] = qIsNaN(vertices[i].altitude()) ? 0.0 : vertices[i].altitude();
            }
        }

        // Close the ring if needed
        if (needsClosing) {
            padfX[nVertices - 1] = vertices.first().longitude();
            padfY[nVertices - 1] = vertices.first().latitude();
            if (useZ) {
                padfZ[nVertices - 1] = qIsNaN(vertices.first().altitude()) ? 0.0 : vertices.first().altitude();
            }
        }

        SHPObject *shpObject = SHPCreateObject(
            shapeType,
            -1,                     // iShape: -1 for new shape
            0,                      // nParts
            nullptr,                // panPartStart
            nullptr,                // panPartType
            nVertices,
            padfX.data(),
            padfY.data(),
            useZ ? padfZ.data() : nullptr,
            nullptr                 // padfM
        );

        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to create polygon shape object";
            continue;
        }

        const int shapeId = SHPWriteObject(shpHandle, -1, shpObject);
        SHPDestroyObject(shpObject);

        if (shapeId < 0) {
            qCWarning(SHPHelperLog) << "Failed to write polygon shape";
        }
    }

    // Write the PRJ file
    if (!writePrjFile(shpFile, errorString)) {
        return false;
    }

    qCDebug(SHPHelperLog) << "Saved" << polygons.size() << "polygon(s) to" << shpFile;
    return true;
}

bool SHPHelper::savePolygonWithHolesToFile(const QString &shpFile, const QGeoPolygon &polygon, QString &errorString)
{
    return savePolygonsWithHolesToFile(shpFile, {polygon}, errorString);
}

bool SHPHelper::savePolygonsWithHolesToFile(const QString &shpFile, const QList<QGeoPolygon> &polygons, QString &errorString)
{
    errorString.clear();

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No polygons to save."));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateGeoPolygonListCoordinates(polygons, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        return false;
    }

    // Check if any coordinate has altitude - use PolygonZ if so
    bool useZ = false;
    for (const QGeoPolygon &poly : polygons) {
        if (GeoUtilities::hasAnyAltitude(poly.perimeter())) {
            useZ = true;
            break;
        }
        for (int h = 0; h < poly.holesCount(); h++) {
            if (GeoUtilities::hasAnyAltitude(poly.holePath(h))) {
                useZ = true;
                break;
            }
        }
        if (useZ) break;
    }
    const int shapeType = useZ ? SHPT_POLYGONZ : SHPT_POLYGON;

    SHPHandle shpHandle = SHPCreate(shpFile.toUtf8().constData(), shapeType);
    if (shpHandle == nullptr) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "Unable to create SHP file."));
        return false;
    }

    auto cleanup = qScopeGuard([&]() {
        SHPClose(shpHandle);
    });

    for (const QGeoPolygon &geoPolygon : polygons) {
        const QList<QGeoCoordinate> perimeter = geoPolygon.perimeter();
        if (perimeter.count() < 3) {
            qCWarning(SHPHelperLog) << "Skipping polygon with less than 3 vertices";
            continue;
        }

        const int nParts = 1 + static_cast<int>(geoPolygon.holesCount());

        // Calculate total vertices (each ring needs closing vertex)
        int totalVertices = 0;
        const bool outerNeedsClosing = (perimeter.first() != perimeter.last());
        totalVertices += static_cast<int>(perimeter.count()) + (outerNeedsClosing ? 1 : 0);

        QVector<bool> holeNeedsClosing(geoPolygon.holesCount());
        for (int h = 0; h < geoPolygon.holesCount(); h++) {
            const QList<QGeoCoordinate> holePath = geoPolygon.holePath(h);
            holeNeedsClosing[h] = (holePath.first() != holePath.last());
            totalVertices += static_cast<int>(holePath.count()) + (holeNeedsClosing[h] ? 1 : 0);
        }

        QVector<int> panPartStart(nParts);
        QVector<double> padfX(totalVertices);
        QVector<double> padfY(totalVertices);
        QVector<double> padfZ(useZ ? totalVertices : 0);

        int vertexIdx = 0;

        // Outer ring (part 0)
        panPartStart[0] = vertexIdx;
        for (int i = 0; i < perimeter.count(); i++) {
            padfX[vertexIdx] = perimeter[i].longitude();
            padfY[vertexIdx] = perimeter[i].latitude();
            if (useZ) {
                padfZ[vertexIdx] = qIsNaN(perimeter[i].altitude()) ? 0.0 : perimeter[i].altitude();
            }
            vertexIdx++;
        }
        if (outerNeedsClosing) {
            padfX[vertexIdx] = perimeter.first().longitude();
            padfY[vertexIdx] = perimeter.first().latitude();
            if (useZ) {
                padfZ[vertexIdx] = qIsNaN(perimeter.first().altitude()) ? 0.0 : perimeter.first().altitude();
            }
            vertexIdx++;
        }

        // Inner rings (holes) - parts 1..N
        for (int h = 0; h < geoPolygon.holesCount(); h++) {
            panPartStart[1 + h] = vertexIdx;
            const QList<QGeoCoordinate> holePath = geoPolygon.holePath(h);
            for (int i = 0; i < holePath.count(); i++) {
                padfX[vertexIdx] = holePath[i].longitude();
                padfY[vertexIdx] = holePath[i].latitude();
                if (useZ) {
                    padfZ[vertexIdx] = qIsNaN(holePath[i].altitude()) ? 0.0 : holePath[i].altitude();
                }
                vertexIdx++;
            }
            if (holeNeedsClosing[h]) {
                padfX[vertexIdx] = holePath.first().longitude();
                padfY[vertexIdx] = holePath.first().latitude();
                if (useZ) {
                    padfZ[vertexIdx] = qIsNaN(holePath.first().altitude()) ? 0.0 : holePath.first().altitude();
                }
                vertexIdx++;
            }
        }

        SHPObject *shpObject = SHPCreateObject(
            shapeType,
            -1,                     // iShape: -1 for new shape
            nParts,
            panPartStart.data(),
            nullptr,                // panPartType
            totalVertices,
            padfX.data(),
            padfY.data(),
            useZ ? padfZ.data() : nullptr,
            nullptr                 // padfM
        );

        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to create polygon shape object";
            continue;
        }

        const int shapeId = SHPWriteObject(shpHandle, -1, shpObject);
        SHPDestroyObject(shpObject);

        if (shapeId < 0) {
            qCWarning(SHPHelperLog) << "Failed to write polygon shape";
        }
    }

    // Write the PRJ file
    if (!writePrjFile(shpFile, errorString)) {
        return false;
    }

    qCDebug(SHPHelperLog) << "Saved" << polygons.size() << "polygon(s) with holes to" << shpFile;
    return true;
}

bool SHPHelper::savePolylineToFile(const QString &shpFile, const QList<QGeoCoordinate> &coords, QString &errorString)
{
    return savePolylinesToFile(shpFile, {coords}, errorString);
}

bool SHPHelper::savePolylinesToFile(const QString &shpFile, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    errorString.clear();

    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No polylines to save."));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validatePolylineListCoordinates(polylines, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        return false;
    }

    // Check if any coordinate has altitude - use ArcZ if so
    const bool useZ = GeoUtilities::hasAnyAltitude(polylines);
    const int shapeType = useZ ? SHPT_ARCZ : SHPT_ARC;

    SHPHandle shpHandle = SHPCreate(shpFile.toUtf8().constData(), shapeType);
    if (shpHandle == nullptr) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "Unable to create SHP file."));
        return false;
    }

    auto cleanup = qScopeGuard([&]() {
        SHPClose(shpHandle);
    });

    for (const QList<QGeoCoordinate> &coords : polylines) {
        if (coords.count() < 2) {
            qCWarning(SHPHelperLog) << "Skipping polyline with less than 2 vertices";
            continue;
        }

        const int nVertices = static_cast<int>(coords.count());

        QVector<double> padfX(nVertices);
        QVector<double> padfY(nVertices);
        QVector<double> padfZ(useZ ? nVertices : 0);

        for (int i = 0; i < nVertices; i++) {
            padfX[i] = coords[i].longitude();
            padfY[i] = coords[i].latitude();
            if (useZ) {
                padfZ[i] = qIsNaN(coords[i].altitude()) ? 0.0 : coords[i].altitude();
            }
        }

        SHPObject *shpObject = SHPCreateObject(
            shapeType,
            -1,                     // iShape: -1 for new shape
            0,                      // nParts
            nullptr,                // panPartStart
            nullptr,                // panPartType
            nVertices,
            padfX.data(),
            padfY.data(),
            useZ ? padfZ.data() : nullptr,
            nullptr                 // padfM
        );

        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to create polyline shape object";
            continue;
        }

        const int shapeId = SHPWriteObject(shpHandle, -1, shpObject);
        SHPDestroyObject(shpObject);

        if (shapeId < 0) {
            qCWarning(SHPHelperLog) << "Failed to write polyline shape";
        }
    }

    // Write the PRJ file
    if (!writePrjFile(shpFile, errorString)) {
        return false;
    }

    qCDebug(SHPHelperLog) << "Saved" << polylines.size() << "polyline(s) to" << shpFile;
    return true;
}

bool SHPHelper::savePointToFile(const QString &shpFile, const QGeoCoordinate &point, QString &errorString)
{
    return savePointsToFile(shpFile, QList<QGeoCoordinate>() << point, errorString);
}

bool SHPHelper::savePointsToFile(const QString &shpFile, const QList<QGeoCoordinate> &points, QString &errorString)
{
    errorString.clear();

    if (points.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "No points to save."));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(points, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),validationError);
        return false;
    }

    // Check if any coordinate has altitude - use PointZ if so
    const bool useZ = GeoUtilities::hasAnyAltitude(points);
    const int shapeType = useZ ? SHPT_POINTZ : SHPT_POINT;

    SHPHandle shpHandle = SHPCreate(shpFile.toUtf8().constData(), shapeType);
    if (shpHandle == nullptr) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QT_TRANSLATE_NOOP("SHP", "Unable to create SHP file."));
        return false;
    }

    auto cleanup = qScopeGuard([&]() {
        SHPClose(shpHandle);
    });

    for (const QGeoCoordinate &coord : points) {
        double x = coord.longitude();
        double y = coord.latitude();
        double z = useZ ? (qIsNaN(coord.altitude()) ? 0.0 : coord.altitude()) : 0.0;

        SHPObject *shpObject = SHPCreateObject(
            shapeType,
            -1,                     // iShape: -1 for new shape
            0,                      // nParts
            nullptr,                // panPartStart
            nullptr,                // panPartType
            1,                      // nVertices
            &x,
            &y,
            useZ ? &z : nullptr,
            nullptr                 // padfM
        );

        if (shpObject == nullptr) {
            qCWarning(SHPHelperLog) << "Failed to create point shape object";
            continue;
        }

        const int shapeId = SHPWriteObject(shpHandle, -1, shpObject);
        SHPDestroyObject(shpObject);

        if (shapeId < 0) {
            qCWarning(SHPHelperLog) << "Failed to write point shape";
        }
    }

    // Write the PRJ file
    if (!writePrjFile(shpFile, errorString)) {
        return false;
    }

    qCDebug(SHPHelperLog) << "Saved" << points.size() << "point(s) to" << shpFile;
    return true;
}

bool SHPHelper::writePrjFile(const QString &shpFile, QString &errorString)
{
    // WGS84 PRJ file content - standard projection for GPS coordinates
    constexpr const char* wgs84PrjContent =
        "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.0,298.257223563]],"
        "PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.017453292519943295]]";

    const QString prjFilename = _prjPathFromShpPath(shpFile);
    return GeoFileIO::saveText(prjFilename, QString::fromLatin1(wgs84PrjContent),
                               QString::fromLatin1(kFormatName), errorString);
}
