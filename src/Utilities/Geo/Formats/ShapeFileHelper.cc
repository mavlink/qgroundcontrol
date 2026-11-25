#include "ShapeFileHelper.h"
#include "KMLHelper.h"
#include "SHPFileHelper.h"

namespace {
constexpr const char* _shapeFileErrorPrefix = QT_TRANSLATE_NOOP("ShapeFileHelper", "Shape file load failed. %1");

/// What the helper of one file format offers.
struct ShapeLoader
{
    const char* extension;
    ShapeFileHelper::ShapeType (*determineShapeType)(const QString&, QString&);
    int (*getEntityCount)(const QString&, QString&);
    bool (*loadPolygons)(const QString&, QList<QList<QGeoCoordinate>>&, QString&, double);
    bool (*loadPolylines)(const QString&, QList<QList<QGeoCoordinate>>&, QString&, double);
    bool (*loadPoints)(const QString&, QList<QGeoCoordinate>&, QString&);
};

constexpr ShapeLoader shapeLoaders[] = {
    {ShapeFileHelper::kmlFileExtension, KMLHelper::determineShapeType, KMLHelper::getEntityCount,
     KMLHelper::loadPolygonsFromFile, KMLHelper::loadPolylinesFromFile, KMLHelper::loadPointsFromFile},
    {ShapeFileHelper::shpFileExtension, SHPFileHelper::determineShapeType, SHPFileHelper::getEntityCount,
     SHPFileHelper::loadPolygonsFromFile, SHPFileHelper::loadPolylinesFromFile, SHPFileHelper::loadPointsFromFile},
};

/// @return the loader for the format that the extension of @a file names, or nullptr with @a errorString set
const ShapeLoader* shapeLoaderFor(const QString& file, QString& errorString)
{
    errorString.clear();

    for (const ShapeLoader& loader : shapeLoaders) {
        if (file.endsWith(loader.extension, Qt::CaseInsensitive)) {
            return &loader;
        }
    }

    // Strip leading dots for user-friendly error message
    const QString kmlExt = QString(ShapeFileHelper::kmlFileExtension).mid(1);
    const QString shpExt = QString(ShapeFileHelper::shpFileExtension).mid(1);
    errorString =
        ShapeFileHelper::tr(_shapeFileErrorPrefix)
            .arg(ShapeFileHelper::tr("Unsupported file type. Only %1 and %2 are supported.").arg(kmlExt, shpExt));
    return nullptr;
}
}  // namespace

ShapeFileHelper::ShapeType ShapeFileHelper::determineShapeType(const QString &file, QString &errorString)
{
    const ShapeLoader* const loader = shapeLoaderFor(file, errorString);
    return loader ? loader->determineShapeType(file, errorString) : ShapeType::Error;
}

int ShapeFileHelper::getEntityCount(const QString &file, QString &errorString)
{
    const ShapeLoader* const loader = shapeLoaderFor(file, errorString);
    return loader ? loader->getEntityCount(file, errorString) : -1;
}

bool ShapeFileHelper::loadPolygonsFromFile(const QString &file, QList<QList<QGeoCoordinate>> &polygons, QString &errorString, double filterMeters)
{
    polygons.clear();

    const ShapeLoader* const loader = shapeLoaderFor(file, errorString);
    return loader && loader->loadPolygons(file, polygons, errorString, filterMeters);
}

bool ShapeFileHelper::loadPolylinesFromFile(const QString &file, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    polylines.clear();

    const ShapeLoader* const loader = shapeLoaderFor(file, errorString);
    return loader && loader->loadPolylines(file, polylines, errorString, filterMeters);
}

bool ShapeFileHelper::loadPointsFromFile(const QString &file, QList<QGeoCoordinate> &points, QString &errorString)
{
    points.clear();

    const ShapeLoader* const loader = shapeLoaderFor(file, errorString);
    return loader && loader->loadPoints(file, points, errorString);
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
