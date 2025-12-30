/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoJsonHelper.h"
#include "JsonHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QJsonValue>
#include <QtLocation/private/qgeojson_p.h>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPath>
#include <QtPositioning/QGeoPolygon>

QGC_LOGGING_CATEGORY(GeoJsonHelperLog, "Utilities.GeoJsonHelper")

namespace GeoJsonHelper
{
    QJsonDocument _loadFile(const QString &filePath, QString &errorString);

    constexpr const char *_errorPrefix = QT_TR_NOOP("GeoJson file load failed. %1");
}

QJsonDocument GeoJsonHelper::_loadFile(const QString &filePath, QString &errorString)
{
    errorString.clear();

    QFile file(filePath);
    if (!file.exists()) {
        errorString = QString(_errorPrefix).arg(
            QString(QT_TRANSLATE_NOOP("GeoJson", "File not found: %1")).arg(filePath));
        return QJsonDocument();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        errorString = QString(_errorPrefix).arg(
            QString(QT_TRANSLATE_NOOP("GeoJson", "Unable to open file: %1 error: %2"))
                .arg(filePath, file.errorString()));
        return QJsonDocument();
    }

    QJsonDocument jsonDoc;
    const QByteArray bytes = file.readAll();
    if (!JsonHelper::isJsonFile(bytes, jsonDoc, errorString)) {
        errorString = QString(_errorPrefix).arg(errorString);
    }

    return jsonDoc;
}

ShapeFileHelper::ShapeType GeoJsonHelper::determineShapeType(const QString &filePath, QString &errorString)
{
    using ShapeType = ShapeFileHelper::ShapeType;

    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return ShapeType::Error;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonDoc);
    if (shapes.isEmpty()) {
        errorString = QString(_errorPrefix).arg(
            QT_TRANSLATE_NOOP("GeoJson", "No shapes found in GeoJson file."));
        return ShapeType::Error;
    }

    for (const QVariant &shapeVar : shapes) {
        if (shapeVar.canConvert<QGeoShape>()) {
            const QGeoShape shape = shapeVar.value<QGeoShape>();
            if (shape.type() == QGeoShape::PathType) {
                return ShapeType::Polyline;
            } else if (shape.type() == QGeoShape::PolygonType) {
                return ShapeType::Polygon;
            }
        }
    }

    errorString = QString(_errorPrefix).arg(
        QT_TRANSLATE_NOOP("GeoJson", "No supported type found in GeoJson file."));
    return ShapeType::Error;
}

bool GeoJsonHelper::loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    errorString.clear();
    vertices.clear();

    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return false;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonDoc);
    if (shapes.isEmpty()) {
        errorString = QString(_errorPrefix).arg(
            QT_TRANSLATE_NOOP("GeoJson", "No polygon data found in GeoJson file."));
        return false;
    }

    for (const QVariant &shapeVar : shapes) {
        if (shapeVar.canConvert<QGeoPolygon>()) {
            const QGeoPolygon poly = shapeVar.value<QGeoPolygon>();
            vertices = poly.perimeter();
            return true;
        }
    }

    errorString = QString(_errorPrefix).arg(
        QT_TRANSLATE_NOOP("GeoJson", "No polygon found in GeoJson file."));
    return false;
}

bool GeoJsonHelper::loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString)
{
    errorString.clear();
    coords.clear();

    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return false;
    }

    const QVariantList shapes = QGeoJson::importGeoJson(jsonDoc);
    if (shapes.isEmpty()) {
        errorString = QString(_errorPrefix).arg(
            QT_TRANSLATE_NOOP("GeoJson", "No polyline data found in GeoJson file."));
        return false;
    }

    for (const QVariant &shapeVar : shapes) {
        if (shapeVar.canConvert<QGeoPath>()) {
            const QGeoPath path = shapeVar.value<QGeoPath>();
            coords = path.path();
            return true;
        }
    }

    errorString = QString(_errorPrefix).arg(
        QT_TRANSLATE_NOOP("GeoJson", "No polyline found in GeoJson file."));
    return false;
}

bool GeoJsonHelper::loadGeoJsonCoordinate(const QJsonValue &jsonValue, bool altitudeRequired, QGeoCoordinate &coordinate, QString &errorString)
{
    return JsonHelper::loadGeoCoordinate(jsonValue, altitudeRequired, coordinate, errorString, true /* geoJsonFormat */);
}

void GeoJsonHelper::saveGeoJsonCoordinate(const QGeoCoordinate &coordinate, bool writeAltitude, QJsonValue &jsonValue)
{
    JsonHelper::saveGeoCoordinate(coordinate, writeAltitude, jsonValue, true /* geoJsonFormat */);
}
