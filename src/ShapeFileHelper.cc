/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ShapeFileHelper.h"
#include "AppSettings.h"
#include "KMLHelper.h"
#include "SHPFileHelper.h"

#include <QFile>

const char* ShapeFileHelper::_errorPrefix = QT_TR_NOOP("Shape file load failed. %1");

QVariantList ShapeFileHelper::determineShapeType(const QString& file)
{
    QString errorString;
    ShapeType shapeType = determineShapeType(file, errorString);

    QVariantList varList;
    varList.append(QVariant::fromValue(shapeType));
    varList.append(QVariant::fromValue(errorString));

    return varList;
}

bool ShapeFileHelper::_fileIsKML(const QString& file, QString& errorString)
{
    errorString.clear();

    if (file.endsWith(AppSettings::kmlFileExtension)) {
        return true;
    } else if (file.endsWith(AppSettings::shpFileExtension)) {
        return false;
    } else {
        errorString = QString(_errorPrefix).arg(tr("Unsupported file type. Only .%1 and .%2 are supported.").arg(AppSettings::kmlFileExtension).arg(AppSettings::shpFileExtension));
    }

    return true;
}

ShapeFileHelper::ShapeType ShapeFileHelper::determineShapeType(const QString& file, QString& errorString)
{
    ShapeType shapeType = Error;

    errorString.clear();

    bool fileIsKML = _fileIsKML(file, errorString);
    if (errorString.isEmpty()) {
        if (fileIsKML) {
            shapeType = KMLHelper::determineShapeType(file, errorString);
        } else {
            shapeType = SHPFileHelper::determineShapeType(file, errorString);
        }
    }

    return shapeType;
}

bool ShapeFileHelper::loadPolygonFromFile(const QString& file, QList<QGeoCoordinate>& vertices, QString& errorString)
{
    bool success = false;

    errorString.clear();
    vertices.clear();

    bool fileIsKML = _fileIsKML(file, errorString);
    if (errorString.isEmpty()) {
        if (fileIsKML) {
            success = KMLHelper::loadPolygonFromFile(file, vertices, errorString);
        } else {
            success = SHPFileHelper::loadPolygonFromFile(file, vertices, errorString);
        }
    }

    return success;
}

bool ShapeFileHelper::loadPolylineFromFile(const QString& file, QList<QGeoCoordinate>& coords, QString& errorString)
{
    errorString.clear();
    coords.clear();

    bool fileIsKML = _fileIsKML(file, errorString);
    if (errorString.isEmpty()) {
        if (fileIsKML) {
            KMLHelper::loadPolylineFromFile(file, coords, errorString);
        } else {
            errorString = QString(_errorPrefix).arg(tr("Polyline not support from SHP files."));
        }
    }

    return errorString.isEmpty();
}

QStringList ShapeFileHelper::fileDialogKMLFilters(void) const
{
    return QStringList(tr("KML Files (*.%1)").arg(AppSettings::kmlFileExtension));
}

QStringList ShapeFileHelper::fileDialogKMLOrSHPFilters(void) const
{
    return QStringList(tr("KML/SHP Files (*.%1 *.%2)").arg(AppSettings::kmlFileExtension).arg(AppSettings::shpFileExtension));
}
