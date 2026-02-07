#include "ShapeFileHelper.h"
#include "KMLHelper.h"
#include "SHPFileHelper.h"
#include <QtCore/QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(ShapeFileHelperLog, "Utilities.ShapeFileHelper")

namespace {
    constexpr const char *_errorPrefix = QT_TR_NOOP("Shape file load failed. %1");
}

ShapeFileHelper::ShapeFileType ShapeFileHelper::_getShapeFileType(const QString &file, QString &errorString)
{
    errorString.clear();

    if (file.endsWith(kmlFileExtension, Qt::CaseInsensitive)) {
        return ShapeFileType::KML;
    } else if (file.endsWith(shpFileExtension, Qt::CaseInsensitive)) {
        return ShapeFileType::SHP;
    } else {
        // Strip leading dots for user-friendly error message
        const QString kmlExt = QString(kmlFileExtension).mid(1);
        const QString shpExt = QString(shpFileExtension).mid(1);
        errorString = QString(_errorPrefix).arg(tr("Unsupported file type. Only %1 and %2 are supported.").arg(kmlExt, shpExt));
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
    case ShapeFileType::None:
    default:
        return ShapeType::Error;
    }
}

bool ShapeFileHelper::loadPolygonFromFile(const QString &file, QList<QGeoCoordinate> &vertices, QString &errorString, double filterMeters)
{
    errorString.clear();
    vertices.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::loadPolygonFromFile(file, vertices, errorString, filterMeters);
    case ShapeFileType::SHP:
        return SHPFileHelper::loadPolygonFromFile(file, vertices, errorString, filterMeters);
    case ShapeFileType::None:
    default:
        return false;
    }
}

bool ShapeFileHelper::loadPolylineFromFile(const QString &file, QList<QGeoCoordinate> &coords, QString &errorString, double filterMeters)
{
    errorString.clear();
    coords.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::loadPolylineFromFile(file, coords, errorString, filterMeters);
    case ShapeFileType::SHP:
        return SHPFileHelper::loadPolylineFromFile(file, coords, errorString, filterMeters);
    case ShapeFileType::None:
    default:
        return false;
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
    static const QStringList filters = QStringList(tr("KML/SHP Files (*%1 *%2)").arg(kmlFileExtension, shpFileExtension));
    return filters;
}
