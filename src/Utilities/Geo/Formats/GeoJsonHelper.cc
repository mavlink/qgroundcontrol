#include "GeoJsonHelper.h"
#include "JsonParsing.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>
#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtLocation/private/qgeojson_p.h>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPath>
#include <QtPositioning/QGeoPolygon>

QGC_LOGGING_CATEGORY(GeoJsonHelperLog, "Utilities.GeoJsonHelper")

namespace GeoJsonHelper
{
    QJsonDocument _loadFile(const QString &filePath, QString &errorString);
    QVariantList _extractShapeValues(const QVariantList &values);
    void _extractShapeValuesRecursive(const QVariant &value, QVariantList &shapes, int depth = 0);

    constexpr int _maxRecursionDepth = 32;
    constexpr const char *_errorPrefix = QT_TR_NOOP("GeoJson file load failed. %1");
}

void GeoJsonHelper::_extractShapeValuesRecursive(const QVariant &value, QVariantList &shapes, int depth)
{
    if (depth >= _maxRecursionDepth) {
        return;
    }

    if (value.canConvert<QGeoPolygon>() || value.canConvert<QGeoPath>() || value.canConvert<QGeoShape>()) {
        (void) shapes.append(value);
    }

    if (value.typeId() == QMetaType::QVariantList) {
        const QVariantList children = value.toList();
        for (const QVariant &child : children) {
            _extractShapeValuesRecursive(child, shapes, depth + 1);
        }
    } else if (value.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = value.toMap();
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            _extractShapeValuesRecursive(it.value(), shapes, depth + 1);
        }
    }
}

QVariantList GeoJsonHelper::_extractShapeValues(const QVariantList &values)
{
    QVariantList shapes;
    for (const QVariant &value : values) {
        _extractShapeValuesRecursive(value, shapes);
    }
    return shapes;
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
    if (!JsonParsing::isJsonFile(bytes, jsonDoc, errorString)) {
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

    const QVariantList imported = QGeoJson::importGeoJson(jsonDoc);
    const QVariantList shapes = _extractShapeValues(imported);
    if (shapes.isEmpty()) {
        errorString = QString(_errorPrefix).arg(
            QT_TRANSLATE_NOOP("GeoJson", "No shapes found in GeoJson file."));
        return ShapeType::Error;
    }

    for (const QVariant &shapeVar : shapes) {
        if (shapeVar.canConvert<QGeoPolygon>()) {
            return ShapeType::Polygon;
        }
        if (shapeVar.canConvert<QGeoPath>()) {
            return ShapeType::Polyline;
        }
        if (shapeVar.canConvert<QGeoShape>()) {
            const QGeoShape shape = shapeVar.value<QGeoShape>();
            if (shape.type() == QGeoShape::PolygonType) {
                return ShapeType::Polygon;
            }
            if (shape.type() == QGeoShape::PathType) {
                return ShapeType::Polyline;
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

    const QVariantList imported = QGeoJson::importGeoJson(jsonDoc);
    const QVariantList shapes = _extractShapeValues(imported);
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
        if (shapeVar.canConvert<QGeoShape>()) {
            const QGeoShape shape = shapeVar.value<QGeoShape>();
            if (shape.type() == QGeoShape::PolygonType) {
                const QGeoPolygon poly(shape);
                vertices = poly.perimeter();
                if (!vertices.isEmpty()) {
                    return true;
                }
            }
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

    const QVariantList imported = QGeoJson::importGeoJson(jsonDoc);
    const QVariantList shapes = _extractShapeValues(imported);
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
        if (shapeVar.canConvert<QGeoShape>()) {
            const QGeoShape shape = shapeVar.value<QGeoShape>();
            if (shape.type() == QGeoShape::PathType) {
                const QGeoPath path(shape);
                coords = path.path();
                if (!coords.isEmpty()) {
                    return true;
                }
            }
        }
    }

    errorString = QString(_errorPrefix).arg(
        QT_TRANSLATE_NOOP("GeoJson", "No polyline found in GeoJson file."));
    return false;
}

bool GeoJsonHelper::loadGeoJsonCoordinate(const QJsonValue &jsonValue, bool altitudeRequired, QGeoCoordinate &coordinate, QString &errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QObject::tr("value for coordinate is not array");
        return false;
    }

    const QJsonArray coordinateArray = jsonValue.toArray();
    const int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QObject::tr("Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    for (const QJsonValue &coordinateValue : coordinateArray) {
        if ((coordinateValue.type() != QJsonValue::Double) && (coordinateValue.type() != QJsonValue::Null)) {
            errorString =
                QObject::tr("Coordinate array may only contain double values, found: %1").arg(coordinateValue.type());
            return false;
        }
    }

    // GeoJSON ordering is [lon, lat, alt] (RFC 7946).
    coordinate = QGeoCoordinate(coordinateArray[1].toDouble(), coordinateArray[0].toDouble());
    if (altitudeRequired) {
        coordinate.setAltitude(JsonParsing::possibleNaNJsonValue(coordinateArray[2]));
    }

    return true;
}

void GeoJsonHelper::saveGeoJsonCoordinate(const QGeoCoordinate &coordinate, bool writeAltitude, QJsonValue &jsonValue)
{
    QJsonArray coordinateArray;
    coordinateArray << coordinate.longitude() << coordinate.latitude();
    if (writeAltitude) {
        coordinateArray << coordinate.altitude();
    }
    jsonValue = QJsonValue(coordinateArray);
}

bool GeoJsonHelper::loadGeoCoordinate(const QJsonValue &jsonValue, bool altitudeRequired, QGeoCoordinate &coordinate,
                                      QString &errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QObject::tr("value for coordinate is not array");
        return false;
    }

    const QJsonArray coordinateArray = jsonValue.toArray();
    const int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QObject::tr("Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    for (const QJsonValue &coordinateValue : coordinateArray) {
        if ((coordinateValue.type() != QJsonValue::Double) && (coordinateValue.type() != QJsonValue::Null)) {
            errorString =
                QObject::tr("Coordinate array may only contain double values, found: %1").arg(coordinateValue.type());
            return false;
        }
    }

    coordinate = QGeoCoordinate(
        JsonParsing::possibleNaNJsonValue(coordinateArray[0]),
        JsonParsing::possibleNaNJsonValue(coordinateArray[1]));

    if (altitudeRequired) {
        coordinate.setAltitude(JsonParsing::possibleNaNJsonValue(coordinateArray[2]));
    }

    return true;
}

void GeoJsonHelper::saveGeoCoordinate(const QGeoCoordinate &coordinate, bool writeAltitude, QJsonValue &jsonValue)
{
    QJsonArray coordinateArray;
    coordinateArray << coordinate.latitude() << coordinate.longitude();

    if (writeAltitude) {
        coordinateArray << coordinate.altitude();
    }

    jsonValue = QJsonValue(coordinateArray);
}

bool GeoJsonHelper::loadGeoCoordinateArray(const QJsonValue &jsonValue, bool altitudeRequired,
                                           QVariantList &rgVarPoints, QString &errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QObject::tr("value for coordinate array is not array");
        return false;
    }

    const QJsonArray rgJsonPoints = jsonValue.toArray();

    rgVarPoints.clear();
    for (const QJsonValue &point : rgJsonPoints) {
        QGeoCoordinate coordinate;
        if (!loadGeoCoordinate(point, altitudeRequired, coordinate, errorString)) {
            return false;
        }
        rgVarPoints.append(QVariant::fromValue(coordinate));
    }

    return true;
}

bool GeoJsonHelper::loadGeoCoordinateArray(const QJsonValue &jsonValue, bool altitudeRequired,
                                           QList<QGeoCoordinate> &rgPoints, QString &errorString)
{
    QVariantList rgVarPoints;

    if (!loadGeoCoordinateArray(jsonValue, altitudeRequired, rgVarPoints, errorString)) {
        return false;
    }

    rgPoints.clear();
    for (const QVariant &point : rgVarPoints) {
        rgPoints.append(point.value<QGeoCoordinate>());
    }

    return true;
}

void GeoJsonHelper::saveGeoCoordinateArray(const QVariantList &rgVarPoints, bool writeAltitude, QJsonValue &jsonValue)
{
    QJsonArray rgJsonPoints;
    for (const QVariant &point : rgVarPoints) {
        QJsonValue jsonPoint;
        saveGeoCoordinate(point.value<QGeoCoordinate>(), writeAltitude, jsonPoint);
        rgJsonPoints.append(jsonPoint);
    }

    jsonValue = rgJsonPoints;
}

void GeoJsonHelper::saveGeoCoordinateArray(const QList<QGeoCoordinate> &rgPoints, bool writeAltitude,
                                           QJsonValue &jsonValue)
{
    QVariantList rgVarPoints;
    for (const QGeoCoordinate &coord : rgPoints) {
        rgVarPoints.append(QVariant::fromValue(coord));
    }

    saveGeoCoordinateArray(rgVarPoints, writeAltitude, jsonValue);
}
