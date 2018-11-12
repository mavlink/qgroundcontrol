/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SHPFileHelper.h"

#include <QFile>
#include <QVariant>
#include <QtDebug>

const char* SHPFileHelper::_errorPrefix = QT_TR_NOOP("SHP file load failed. %1");

bool SHPFileHelper::_validateSHPFiles(const QString& shpFile, QString& errorString)
{
    errorString.clear();

    if (shpFile.endsWith(QStringLiteral(".shp"))) {
        QString prjFilename = shpFile.left(shpFile.length() - 4) + QStringLiteral(".prj");
        QFile prjFile(prjFilename);
        if (prjFile.exists()) {
            if (prjFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream strm(&prjFile);
                QString line = strm.readLine();
                if (!line.startsWith(QStringLiteral("GEOGCS[\"GCS_WGS_1984\","))) {
                    errorString = QString(_errorPrefix).arg(tr("Only WGS84 projections are supported."));
                }
            } else {
                errorString = QString(_errorPrefix).arg(tr("PRJ file open failed: %1").arg(prjFile.errorString()));
            }
        } else {
            errorString = QString(_errorPrefix).arg(tr("File not found: %1").arg(prjFilename));
        }
    } else {
        errorString = QString(_errorPrefix).arg(tr("File is not a .shp file: %1").arg(shpFile));
    }

    return errorString.isEmpty();
}

SHPHandle SHPFileHelper::_loadShape(const QString& shpFile, QString& errorString)
{
    SHPHandle shpHandle = Q_NULLPTR;

    errorString.clear();

    if (_validateSHPFiles(shpFile, errorString)) {
        if (!(shpHandle = SHPOpen(shpFile.toUtf8(), "rb"))) {
            errorString = QString(_errorPrefix).arg(tr("SHPOpen failed."));
        }
    }

    return shpHandle;
}

ShapeFileHelper::ShapeType SHPFileHelper::determineShapeType(const QString& shpFile, QString& errorString)
{
    ShapeFileHelper::ShapeType shapeType = ShapeFileHelper::Error;

    errorString.clear();

    SHPHandle shpHandle = SHPFileHelper::_loadShape(shpFile, errorString);
    if (errorString.isEmpty()) {
        int cEntities, type;

        SHPGetInfo(shpHandle, &cEntities /* pnEntities */, &type, Q_NULLPTR /* padfMinBound */, Q_NULLPTR /* padfMaxBound */);
        qDebug() << "SHPGetInfo" << shpHandle << cEntities << type;
        if (cEntities != 1) {
            errorString = QString(_errorPrefix).arg(tr("More than one entity found."));
        } else if (type == SHPT_POLYGON) {
            shapeType = ShapeFileHelper::Polygon;
        } else {
            errorString = QString(_errorPrefix).arg(tr("No supported types found."));
        }
    }

    SHPClose(shpHandle);

    return shapeType;
}

bool SHPFileHelper::loadPolygonFromFile(const QString& shpFile, QList<QGeoCoordinate>& vertices, QString& errorString)
{
    double      vertexFilterMeters = 5;
    SHPHandle   shpHandle = Q_NULLPTR;
    SHPObject*  shpObject = Q_NULLPTR;

    errorString.clear();
    vertices.clear();

    shpHandle = SHPFileHelper::_loadShape(shpFile, errorString);
    if (!errorString.isEmpty()) {
        goto Error;
    }

    int cEntities, shapeType;
    SHPGetInfo(shpHandle, &cEntities, &shapeType, Q_NULLPTR /* padfMinBound */, Q_NULLPTR /* padfMaxBound */);
    if (shapeType != SHPT_POLYGON) {
        errorString = QString(_errorPrefix).arg(tr("File does not contain a polygon."));
        goto Error;
    }

    shpObject = SHPReadObject(shpHandle, 0);
    if (shpObject->nParts != 1) {
        errorString = QString(_errorPrefix).arg(tr("Only single part polygons are supported."));
        goto Error;
    }

    for (int i=0; i<shpObject->nVertices; i++) {
        vertices.append(QGeoCoordinate(shpObject->padfY[i], shpObject->padfX[i]));
    }

    // Filter last vertex such that it differs from first
    {
        QGeoCoordinate firstVertex = vertices[0];

        while (vertices.count() > 3 && vertices.last().distanceTo(firstVertex) < vertexFilterMeters) {
            vertices.removeLast();
        }
    }

    // Filter vertex distances to be larger than 1 meter apart
    {
        int i = 0;
        while (i < vertices.count() - 2) {
            if (vertices[i].distanceTo(vertices[i+1]) < vertexFilterMeters) {
                vertices.removeAt(i+1);
            } else {
                i++;
            }
        }
    }

Error:
    if (shpObject) {
        SHPDestroyObject(shpObject);
    }
    if (shpHandle) {
        SHPClose(shpHandle);
    }
    return errorString.isEmpty();
}
