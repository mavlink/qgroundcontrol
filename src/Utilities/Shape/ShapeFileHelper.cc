/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ShapeFileHelper.h"
#include "KMLHelper.h"
#include "SHPFileHelper.h"

bool ShapeFileHelper::_fileIsKML(const QString &file, QString &errorString)
{
    return (_getShapeFileType(file, errorString) == ShapeFileType::KML);
}

bool ShapeFileHelper::_fileIsSHP(const QString &file, QString &errorString)
{
    return (_getShapeFileType(file, errorString) == ShapeFileType::SHP);
}

ShapeFileHelper::ShapeFileType ShapeFileHelper::_getShapeFileType(const QString &file, QString &errorString)
{
    errorString.clear();

    if (file.endsWith(kmlFileExtension)) {
        return ShapeFileType::KML;
    } else if (file.endsWith(shpFileExtension)) {
        return ShapeFileType::SHP;
    } else {
        errorString = QString(_errorPrefix).arg(tr("Unsupported file type. Only .%1 and .%2 are supported.").arg(kmlFileExtension, shpFileExtension));
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

bool ShapeFileHelper::loadPolygonFromFile(const QString &file, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    errorString.clear();
    vertices.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::loadPolygonFromFile(file, vertices, errorString);
    case ShapeFileType::SHP:
        return SHPFileHelper::loadPolygonFromFile(file, vertices, errorString);
    case ShapeFileType::None:
    default:
        return false;
    }
}

bool ShapeFileHelper::loadPolylineFromFile(const QString &file, QList<QGeoCoordinate> &coords, QString &errorString)
{
    errorString.clear();
    coords.clear();

    switch (_getShapeFileType(file, errorString)) {
    case ShapeFileType::KML:
        return KMLHelper::loadPolylineFromFile(file, coords, errorString);
    case ShapeFileType::SHP:
        return SHPFileHelper::loadPolylineFromFile(file, coords, errorString);
    case ShapeFileType::None:
    default:
        return false;
    }
}

QStringList ShapeFileHelper::fileDialogKMLFilters()
{
    static const QStringList filters = QStringList(tr("KML Files (*.%1)").arg(kmlFileExtension));
    return filters;
}

QStringList ShapeFileHelper::fileDialogKMLOrSHPFilters()
{
    static const QStringList filters = QStringList(tr("KML/SHP Files (*.%1 *.%2)").arg(kmlFileExtension, shpFileExtension));
    return filters;
}
