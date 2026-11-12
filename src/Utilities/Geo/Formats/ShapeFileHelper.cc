#include "ShapeFileHelper.h"

#include "GeoJsonHelper.h"
#include "KMLHelper.h"
#include "QGCLoggingCategory.h"
#include "SHPFileHelper.h"

QGC_LOGGING_CATEGORY(ShapeFileHelperLog, "Utilities.ShapeFileHelper")

namespace {
constexpr const char* _shapeFileErrorPrefix = QT_TRANSLATE_NOOP("ShapeFileHelper", "Shape file load failed. %1");
}

ShapeFileHelper::ShapeFileType ShapeFileHelper::_getShapeFileType(const QString &file, QString &errorString)
{
    errorString.clear();

    if (file.endsWith(kmlFileExtension, Qt::CaseInsensitive)) {
        return ShapeFileType::KML;
    } else if (file.endsWith(shpFileExtension, Qt::CaseInsensitive)) {
        return ShapeFileType::SHP;
    } else if (file.endsWith(geoJsonFileExtension, Qt::CaseInsensitive)) {
        return ShapeFileType::GeoJson;
    } else {
        // Strip leading dots for user-friendly error message
        const QString kmlExt = QString(kmlFileExtension).mid(1);
        const QString shpExt = QString(shpFileExtension).mid(1);
        const QString geoJsonExt = QString(geoJsonFileExtension).mid(1);
        errorString =
            tr(_shapeFileErrorPrefix)
                .arg(tr("Unsupported file type. Only %1, %2 and %3 are supported.").arg(kmlExt, shpExt, geoJsonExt));
    }

    return ShapeFileType::None;
}

ShapeFileHelper::ShapeType ShapeFileHelper::determineShapeType(const QString &file, QString &errorString)
{
    errorString.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::determineShapeType(file, errorString);
    case ShapeFileType::SHP:
        return SHPFileHelper::determineShapeType(file, errorString);
    case ShapeFileType::GeoJson:
        return GeoJsonHelper::determineShapeType(file, errorString);
    case ShapeFileType::None:
    default:
        return ShapeType::Error;
    }
}

int ShapeFileHelper::getEntityCount(const QString &file, QString &errorString)
{
    errorString.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::getEntityCount(file, errorString);
    case ShapeFileType::SHP:
        return SHPFileHelper::getEntityCount(file, errorString);
    case ShapeFileType::GeoJson:
        return GeoJsonHelper::getEntityCount(file, errorString);
    case ShapeFileType::None:
    default:
        return 0;
    }
}

bool ShapeFileHelper::loadPolygonsFromFile(const QString &file, QList<QList<QGeoCoordinate>> &polygons, QString &errorString, double filterMeters)
{
    errorString.clear();
    polygons.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::loadPolygonsFromFile(file, polygons, errorString, filterMeters);
    case ShapeFileType::SHP:
        return SHPFileHelper::loadPolygonsFromFile(file, polygons, errorString, filterMeters);
    case ShapeFileType::GeoJson:
        return GeoJsonHelper::loadPolygonsFromFile(file, polygons, errorString, filterMeters);
    case ShapeFileType::None:
    default:
        return false;
    }
}

bool ShapeFileHelper::loadPolylinesFromFile(const QString &file, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    errorString.clear();
    polylines.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::loadPolylinesFromFile(file, polylines, errorString, filterMeters);
    case ShapeFileType::SHP:
        return SHPFileHelper::loadPolylinesFromFile(file, polylines, errorString, filterMeters);
    case ShapeFileType::GeoJson:
        return GeoJsonHelper::loadPolylinesFromFile(file, polylines, errorString, filterMeters);
    case ShapeFileType::None:
    default:
        return false;
    }
}

bool ShapeFileHelper::loadPointsFromFile(const QString &file, QList<QGeoCoordinate> &points, QString &errorString)
{
    errorString.clear();
    points.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::loadPointsFromFile(file, points, errorString);
    case ShapeFileType::SHP:
        return SHPFileHelper::loadPointsFromFile(file, points, errorString);
    case ShapeFileType::GeoJson:
        return GeoJsonHelper::loadPointsFromFile(file, points, errorString);
    case ShapeFileType::None:
    default:
        return false;
    }
}

QStringList ShapeFileHelper::fileDialogKMLFilters()
{
    static const QStringList filters = QStringList(tr("KML Files (*%1)").arg(kmlFileExtension));
    return filters;
}

QStringList ShapeFileHelper::fileDialogKMLOrSHPFilters()
{
    static const QStringList filters = QStringList(
        tr("KML/SHP/GeoJSON Files (*%1 *%2 *%3)").arg(kmlFileExtension, shpFileExtension, geoJsonFileExtension));
    return filters;
}
