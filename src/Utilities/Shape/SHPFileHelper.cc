/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SHPFileHelper.h"
#include "QGCGeo.h"
#include "QGCLoggingCategory.h"
#include "shapefil.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>

QGC_LOGGING_CATEGORY(SHPFileHelperLog, "qgc.utilities.shpfilehelper");

namespace SHPFileHelper
{
    /// Validates the specified SHP file is truly a SHP file and is in the format we understand.
    ///     @param utmZone[out] Zone for UTM shape, 0 for lat/lon shape
    ///     @param utmSouthernHemisphere[out] true/false for UTM hemisphere
    /// @return true: Valid supported SHP file found, false: Invalid or unsupported file found
    bool _validateSHPFiles(const QString &shpFile, int *utmZone, bool *utmSouthernHemisphere, QString &errorString);

    /// @param utmZone[out] Zone for UTM shape, 0 for lat/lon shape
    /// @param utmSouthernHemisphere[out] true/false for UTM hemisphere
    SHPHandle _loadShape(const QString &shpFile, int *utmZone, bool *utmSouthernHemisphere, QString &errorString);

    constexpr const char *_errorPrefix = QT_TR_NOOP("SHP file load failed. %1");
};

bool SHPFileHelper::_validateSHPFiles(const QString &shpFile, int *utmZone, bool *utmSouthernHemisphere, QString &errorString)
{
    *utmZone = 0;
    errorString.clear();

    if (!shpFile.endsWith(QStringLiteral(".shp"))) {
        errorString = QString(_errorPrefix).arg(QString(QT_TRANSLATE_NOOP("SHP", "File is not a .shp file: %1")).arg(shpFile));
        return false;
    }

    const QString prjFilename = shpFile.left(shpFile.length() - 4) + QStringLiteral(".prj");
    QFile prjFile(prjFilename);
    if (!prjFile.exists()) {
        errorString = QString(_errorPrefix).arg(QString(QT_TRANSLATE_NOOP("SHP", "File not found: %1")).arg(prjFilename));
        return false;
    }

    if (!prjFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "PRJ file open failed: %1"), prjFile.errorString());
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
            errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "UTM projection is not in supported format. Must be PROJCS[\"WGS_1984_UTM_Zone_##N/S"));
        }
    } else {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "Only WGS84 or UTM projections are supported."));
    }

    return errorString.isEmpty();
}

SHPHandle SHPFileHelper::_loadShape(const QString &shpFile, int *utmZone, bool *utmSouthernHemisphere, QString &errorString)
{
    errorString.clear();

    if (!_validateSHPFiles(shpFile, utmZone, utmSouthernHemisphere, errorString)) {
        return nullptr;
    }

    SAHooks sHooks{};
    SASetupDefaultHooks(&sHooks);
    sHooks.Error = [](const char *message) {
        qCWarning(SHPFileHelperLog) << "SHP Error:" << message;
    };
    // TODO: Replace other hooks and use QFile to be compatible with Qt Resource System

    SHPHandle shpHandle = SHPOpenLL(shpFile.toUtf8().constData(), "rb", &sHooks);
    if (!shpHandle) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "SHPOpen failed."));
    }

    return shpHandle;
}

ShapeFileHelper::ShapeType SHPFileHelper::determineShapeType(const QString &shpFile, QString &errorString)
{
    using ShapeType = ShapeFileHelper::ShapeType;

    ShapeType shapeType = ShapeType::Error;

    errorString.clear();

    int utmZone;
    bool utmSouthernHemisphere;
    SHPHandle shpHandle = SHPFileHelper::_loadShape(shpFile, &utmZone, &utmSouthernHemisphere, errorString);
    if (errorString.isEmpty()) {
        Q_CHECK_PTR(shpHandle);

        int cEntities, type;
        SHPGetInfo(shpHandle, &cEntities /* pnEntities */, &type, nullptr /* padfMinBound */, nullptr /* padfMaxBound */);
        qCDebug(SHPFileHelperLog) << "SHPGetInfo" << shpHandle << cEntities << type;
        if (cEntities != 1) {
            errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "More than one entity found."));
        } else if (type == SHPT_POLYGON) {
            shapeType = ShapeType::Polygon;
        } else if (type == SHPT_ARC) {
            shapeType = ShapeType::Polyline;
        } else {
            errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "No supported types found."));
        }
    }

    if (shpHandle) {
        SHPClose(shpHandle);
    }

    return shapeType;
}

bool SHPFileHelper::loadPolygonFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    static constexpr double vertexFilterMeters = 5;
    int utmZone = 0;
    bool utmSouthernHemisphere = false;
    SHPObject *shpObject = nullptr;

    errorString.clear();
    vertices.clear();

    SHPHandle shpHandle = SHPFileHelper::_loadShape(shpFile, &utmZone, &utmSouthernHemisphere, errorString);
    if (!errorString.isEmpty()) {
        goto Error;
    }
    Q_CHECK_PTR(shpHandle);

    int cEntities, shapeType;
    SHPGetInfo(shpHandle, &cEntities, &shapeType, nullptr /* padfMinBound */, nullptr /* padfMaxBound */);
    if (shapeType != SHPT_POLYGON) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "File does not contain a polygon."));
        goto Error;
    }

    shpObject = SHPReadObject(shpHandle, 0);
    if (!shpObject) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "Failed to read polygon object."));
        goto Error;
    }

    if (shpObject->nParts != 1) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "Only single part polygons are supported."));
        goto Error;
    }

    for (int i = 0; i < shpObject->nVertices; i++) {
        QGeoCoordinate coord;
        if (!utmZone || !QGCGeo::convertUTMToGeo(shpObject->padfX[i], shpObject->padfY[i], utmZone, utmSouthernHemisphere, coord)) {
            coord.setLatitude(shpObject->padfY[i]);
            coord.setLongitude(shpObject->padfX[i]);
        }
        vertices.append(coord);
    }

    // Filter last vertex such that it differs from first
    {
        const QGeoCoordinate firstVertex = vertices[0];
        while ((vertices.count() > 3) && (vertices.last().distanceTo(firstVertex) < vertexFilterMeters)) {
            vertices.removeLast();
        }
    }

    // Filter vertex distances to be larger than 1 meter apart
    {
        int i = 0;
        while (i < (vertices.count() - 2)) {
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

bool SHPFileHelper::loadPolylineFromFile(const QString &shpFile, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    int utmZone = 0;
    bool utmSouthernHemisphere = false;
    SHPObject *shpObject = nullptr;

    errorString.clear();
    vertices.clear();

    SHPHandle shpHandle = SHPFileHelper::_loadShape(shpFile, &utmZone, &utmSouthernHemisphere, errorString);
    if (!errorString.isEmpty()) {
        goto Error;
    }
    Q_CHECK_PTR(shpHandle);

    int cEntities, shapeType;
    SHPGetInfo(shpHandle, &cEntities, &shapeType, nullptr /* padfMinBound */, nullptr /* padfMaxBound */);
    if (shapeType != SHPT_ARC) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "File does not contain a polyline."));
        goto Error;
    }

    shpObject = SHPReadObject(shpHandle, 0);
    if (!shpObject) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "Failed to read polyline object."));
        goto Error;
    }

    if (shpObject->nParts != 1) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("SHP", "Only single part polylines are supported."));
        goto Error;
    }

    for (int i = 0; i < shpObject->nVertices; i++) {
        QGeoCoordinate coord;
        if (!utmZone || !QGCGeo::convertUTMToGeo(shpObject->padfX[i], shpObject->padfY[i], utmZone, utmSouthernHemisphere, coord)) {
            coord.setLatitude(shpObject->padfY[i]);
            coord.setLongitude(shpObject->padfX[i]);
        }
        vertices.append(coord);
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
