#include "GeoJsonHelper.h"

#include <algorithm>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QRegularExpression>
#include <QtCore/QVariantMap>
#include <QtPositioning/QGeoCoordinate>

#include "JsonParsing.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GeoJsonHelperLog, "Utilities.GeoJsonHelper")

namespace GeoJsonHelper {

struct Shape
{
    enum class Kind
    {
        Point,
        Polyline,
        Polygon,
    };

    Kind kind;
    QVariantList positions;
};

struct ShapeRules
{
    const char* typeName;  ///< GeoJSON geometry type
    int minVertices;
};

QJsonDocument _loadFile(const QString& filePath, QString& errorString);
QString _unsupportedCrs(const QJsonObject& root);
QList<Shape> _shapes(const QJsonDocument& jsonDoc);
void _appendShapes(const QVariantMap& geometry, int nestingDepth, QList<Shape>& shapes);
ShapeRules _shapeRules(Shape::Kind kind);
bool _loadShapes(const QString& filePath, Shape::Kind kind, double filterMeters, QList<QList<QGeoCoordinate>>& loaded,
                 QString& errorString);
QString _vertices(const Shape& shape, int shapeNumber, QList<QGeoCoordinate>& vertices);
bool _coordinate(const QVariant& positionValue, QGeoCoordinate& coordinate);
bool _number(const QVariant& value, double& number);
void _filterVertices(QList<QGeoCoordinate>& vertices, double filterMeters, int minVertices);
QString _jsonText(const QVariant& value);

// Shapes inside more collections (FeatureCollection, GeometryCollection, Multi* geometry) are ignored.
constexpr int _maxNestingDepth = 15;
constexpr const char* _errorPrefix = QT_TRANSLATE_NOOP("GeoJsonHelper", "GeoJSON file load failed. %1");

}  // namespace GeoJsonHelper

void GeoJsonHelper::_appendShapes(const QVariantMap& geometry, int nestingDepth, QList<Shape>& shapes)
{
    if (nestingDepth > _maxNestingDepth) {
        return;
    }

    const QString type = geometry.value(QStringLiteral("type")).toString();
    const QVariantList coordinates = geometry.value(QStringLiteral("coordinates")).toList();

    if (type == QLatin1String("Point")) {
        shapes.append(Shape{Shape::Kind::Point, {QVariant(coordinates)}});
    } else if (type == QLatin1String("LineString")) {
        shapes.append(Shape{Shape::Kind::Polyline, coordinates});
    } else if (type == QLatin1String("Polygon")) {
        // The first ring is the perimeter, the others are holes.
        shapes.append(Shape{Shape::Kind::Polygon, coordinates.value(0).toList()});
    } else if ((type == QLatin1String("MultiPoint")) || (type == QLatin1String("MultiLineString")) ||
               (type == QLatin1String("MultiPolygon"))) {
        // A Multi* geometry lists the coordinates of geometries of the type named after "Multi".
        const QString memberType = type.sliced(QLatin1String("Multi").size());
        for (const QVariant& memberCoordinates : coordinates) {
            _appendShapes({{QStringLiteral("type"), memberType}, {QStringLiteral("coordinates"), memberCoordinates}},
                          nestingDepth + 1, shapes);
        }
    } else if (type == QLatin1String("GeometryCollection")) {
        const QVariantList geometries = geometry.value(QStringLiteral("geometries")).toList();
        for (const QVariant& member : geometries) {
            _appendShapes(member.toMap(), nestingDepth + 1, shapes);
        }
    }
}

// Returns the shapes of all geometries in document order.
QList<GeoJsonHelper::Shape> GeoJsonHelper::_shapes(const QJsonDocument& jsonDoc)
{
    // Values are converted by QVariant, as in Qt's GeoJSON importer: a number written as a string still counts as
    // a number.
    const QVariantMap root = jsonDoc.object().toVariantMap();
    const QString type = root.value(QStringLiteral("type")).toString();

    QList<Shape> shapes;
    if (type == QLatin1String("FeatureCollection")) {
        const QVariantList features = root.value(QStringLiteral("features")).toList();
        for (const QVariant& feature : features) {
            _appendShapes(feature.toMap().value(QStringLiteral("geometry")).toMap(), 1, shapes);
        }
    } else if (type == QLatin1String("Feature")) {
        _appendShapes(root.value(QStringLiteral("geometry")).toMap(), 0, shapes);
    } else {
        _appendShapes(root, 0, shapes);
    }

    return shapes;
}

GeoJsonHelper::ShapeRules GeoJsonHelper::_shapeRules(Shape::Kind kind)
{
    switch (kind) {
        case Shape::Kind::Polygon:
            return {"Polygon", 3};
        case Shape::Kind::Polyline:
            return {"LineString", 2};
        case Shape::Kind::Point:
            break;
    }

    return {"Point", 1};
}

bool GeoJsonHelper::_number(const QVariant& value, double& number)
{
    // QVariant would convert true and false to 1 and 0.
    if (value.typeId() == QMetaType::Bool) {
        return false;
    }

    bool isNumber = false;
    number = value.toDouble(&isNumber);
    return isNumber;
}

bool GeoJsonHelper::_coordinate(const QVariant& positionValue, QGeoCoordinate& coordinate)
{
    // GeoJSON ordering is [lon, lat, alt] (RFC 7946).
    const QVariantList position = positionValue.toList();
    double longitude = 0.0;
    double latitude = 0.0;
    if ((position.size() < 2) || !_number(position.at(0), longitude) || !_number(position.at(1), latitude)) {
        return false;
    }

    coordinate = QGeoCoordinate(latitude, longitude);
    if (position.size() >= 3) {
        double altitude = 0.0;
        if (!_number(position.at(2), altitude)) {
            return false;
        }
        coordinate.setAltitude(altitude);
    }

    return coordinate.isValid();
}

// The JSON text of a value, shortened to fit a message.
QString GeoJsonHelper::_jsonText(const QVariant& value)
{
    constexpr int maxLength = 60;

    const QByteArray json = QJsonDocument(QJsonArray{QJsonValue::fromVariant(value)}).toJson(QJsonDocument::Compact);
    QString text = QString::fromUtf8(json.sliced(1).chopped(1));
    if (text.size() > maxLength) {
        text.truncate(maxLength);
        text.append(QLatin1String("..."));
    }

    return text;
}

// Converts a shape to the vertices the editors use: a polygon comes without its closing position and wound
// clockwise. Returns what makes the shape unusable, or an empty string.
QString GeoJsonHelper::_vertices(const Shape& shape, int shapeNumber, QList<QGeoCoordinate>& vertices)
{
    const ShapeRules rules = _shapeRules(shape.kind);

    vertices.reserve(shape.positions.size());
    for (const QVariant& position : shape.positions) {
        QGeoCoordinate coordinate;
        if (!_coordinate(position, coordinate)) {
            //: %1 is a GeoJSON geometry type such as Polygon, %2 its number in the file, %3 the position
            return QCoreApplication::translate("GeoJson",
                                               "%1 %2 has an invalid position: %3. "
                                               "A position is [longitude, latitude] in degrees.")
                .arg(QLatin1String(rules.typeName))
                .arg(shapeNumber)
                .arg(_jsonText(position));
        }
        vertices.append(coordinate);
    }

    if (shape.kind == Shape::Kind::Polygon) {
        // The last position of a ring repeats the first.
        if ((vertices.count() > 1) && (vertices.first().latitude() == vertices.last().latitude()) &&
            (vertices.first().longitude() == vertices.last().longitude())) {
            vertices.removeLast();
        }

        // QGC wants clockwise winding, RFC 7946 winds the outer ring counterclockwise.
        double sum = 0;
        for (int i = 0; i < vertices.count(); i++) {
            const QGeoCoordinate& coord1 = vertices[i];
            const QGeoCoordinate& coord2 = vertices[(i + 1) % vertices.count()];
            sum += (coord2.longitude() - coord1.longitude()) * (coord2.latitude() + coord1.latitude());
        }
        if (sum < 0.0) {
            std::reverse(vertices.begin(), vertices.end());
        }
    }

    if (vertices.count() < rules.minVertices) {
        //: %1 is a GeoJSON geometry type such as Polygon, %2 its number in the file, %3 a number of vertices
        return QCoreApplication::translate("GeoJson", "%1 %2 has fewer than %3 vertices.")
            .arg(QLatin1String(rules.typeName))
            .arg(shapeNumber)
            .arg(rules.minVertices);
    }

    return {};
}

void GeoJsonHelper::_filterVertices(QList<QGeoCoordinate>& vertices, double filterMeters, int minVertices)
{
    if (filterMeters <= 0 || vertices.count() <= minVertices) {
        return;
    }

    int i = 0;
    while (i < (vertices.count() - 1)) {
        if ((vertices.count() > minVertices) && (vertices[i].distanceTo(vertices[i + 1]) < filterMeters)) {
            vertices.removeAt(i + 1);
        } else {
            i++;
        }
    }
}

// Returns the coordinate reference system of a document whose positions are not WGS84 longitude and latitude, or an
// empty string. RFC 7946 has no "crs" member, but files written to the 2008 specification can name any system.
QString GeoJsonHelper::_unsupportedCrs(const QJsonObject& root)
{
    const QJsonValue crs = root.value(QLatin1String("crs"));
    if (crs.isUndefined() || crs.isNull()) {
        return {};
    }

    // WGS84 is named "urn:ogc:def:crs:OGC:1.3:CRS84", "urn:ogc:def:crs:EPSG::4326" or "EPSG:4326", for example.
    static const QRegularExpression wgs84Name(QStringLiteral("(^|[:/])(CRS84h?|4326)$"),
                                              QRegularExpression::CaseInsensitiveOption);
    const QString name = crs[QLatin1String("properties")][QLatin1String("name")].toString();
    if (wgs84Name.match(name).hasMatch()) {
        return {};
    }

    return name.isEmpty() ? _jsonText(crs.toVariant()) : name;
}

QJsonDocument GeoJsonHelper::_loadFile(const QString &filePath, QString &errorString)
{
    errorString.clear();

    QFile file(filePath);
    if (!file.exists()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
            QCoreApplication::translate("GeoJson", "File not found: %1").arg(filePath));
        return QJsonDocument();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(
            QCoreApplication::translate("GeoJson", "Unable to open file: %1 error: %2")
                .arg(filePath, file.errorString()));
        return QJsonDocument();
    }

    QJsonDocument jsonDoc;
    const QByteArray bytes = file.readAll();
    if (!JsonParsing::isJsonFile(bytes, jsonDoc, errorString)) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(errorString);
        return jsonDoc;
    }

    const QString unsupportedCrs = _unsupportedCrs(jsonDoc.object());
    if (!unsupportedCrs.isEmpty()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix)
                          .arg(QCoreApplication::translate("GeoJson",
                                                           "Unsupported coordinate reference system: %1. "
                                                           "Convert the file to WGS84 using QGIS or ogr2ogr.")
                                   .arg(unsupportedCrs));
    }

    return jsonDoc;
}

// Loads the shapes of one kind in document order. A shape that cannot be used is skipped, as in KML and SHP files;
// the load fails when none is left, with what is wrong with the first.
bool GeoJsonHelper::_loadShapes(const QString& filePath, Shape::Kind kind, double filterMeters,
                                QList<QList<QGeoCoordinate>>& loaded, QString& errorString)
{
    errorString.clear();
    loaded.clear();

    const QJsonDocument jsonDoc = _loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return false;
    }

    const ShapeRules rules = _shapeRules(kind);
    const QList<Shape> shapes = _shapes(jsonDoc);
    QString firstProblem;
    int shapeNumber = 0;
    for (const Shape& shape : shapes) {
        if (shape.kind != kind) {
            continue;
        }
        shapeNumber++;

        QList<QGeoCoordinate> vertices;
        const QString problem = _vertices(shape, shapeNumber, vertices);
        if (!problem.isEmpty()) {
            qCWarning(GeoJsonHelperLog).noquote() << "Skipping shape:" << problem;
            if (firstProblem.isEmpty()) {
                firstProblem = problem;
            }
            continue;
        }

        _filterVertices(vertices, filterMeters, rules.minVertices);
        loaded.append(vertices);
    }

    if (loaded.isEmpty()) {
        if (firstProblem.isEmpty()) {
            //: %1 is a GeoJSON geometry type such as Polygon
            firstProblem = QCoreApplication::translate("GeoJson", "No %1 found in GeoJSON file.")
                               .arg(QLatin1String(rules.typeName));
        }
        errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix).arg(firstProblem);
        return false;
    }

    return true;
}

ShapeFileHelper::ShapeType GeoJsonHelper::determineShapeType(const QString& filePath, QString& errorString)
{
    using ShapeType = ShapeFileHelper::ShapeType;

    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return ShapeType::Error;
    }

    const QList<Shape> shapes = _shapes(jsonDoc);
    const auto contains = [&shapes](Shape::Kind kind) {
        return std::any_of(shapes.cbegin(), shapes.cend(), [kind](const Shape& shape) { return shape.kind == kind; });
    };

    // Same precedence as for KML files
    if (contains(Shape::Kind::Polygon)) {
        return ShapeType::Polygon;
    }
    if (contains(Shape::Kind::Polyline)) {
        return ShapeType::Polyline;
    }
    if (contains(Shape::Kind::Point)) {
        return ShapeType::Point;
    }

    errorString = QCoreApplication::translate("GeoJsonHelper", _errorPrefix)
                      .arg(QCoreApplication::translate("GeoJson", "No shapes found in GeoJSON file."));
    return ShapeType::Error;
}

int GeoJsonHelper::getEntityCount(const QString& filePath, QString& errorString)
{
    const QJsonDocument jsonDoc = GeoJsonHelper::_loadFile(filePath, errorString);
    if (!errorString.isEmpty()) {
        return 0;
    }

    return static_cast<int>(_shapes(jsonDoc).count());
}

bool GeoJsonHelper::loadPolygonsFromFile(const QString& filePath, QList<QList<QGeoCoordinate>>& polygons,
                                         QString& errorString, double filterMeters)
{
    return _loadShapes(filePath, Shape::Kind::Polygon, filterMeters, polygons, errorString);
}

bool GeoJsonHelper::loadPolylinesFromFile(const QString& filePath, QList<QList<QGeoCoordinate>>& polylines,
                                          QString& errorString, double filterMeters)
{
    return _loadShapes(filePath, Shape::Kind::Polyline, filterMeters, polylines, errorString);
}

bool GeoJsonHelper::loadPointsFromFile(const QString& filePath, QList<QGeoCoordinate>& points, QString& errorString)
{
    points.clear();

    QList<QList<QGeoCoordinate>> loaded;
    if (!_loadShapes(filePath, Shape::Kind::Point, 0.0, loaded, errorString)) {
        return false;
    }

    for (const QList<QGeoCoordinate>& point : std::as_const(loaded)) {
        points.append(point.first());
    }

    return true;
}

bool GeoJsonHelper::loadGeoJsonCoordinate(const QJsonValue &jsonValue, bool altitudeRequired, QGeoCoordinate &coordinate, QString &errorString)
{
    if (!jsonValue.isArray()) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "value for coordinate is not array");
        return false;
    }

    const QJsonArray coordinateArray = jsonValue.toArray();
    const int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    for (const QJsonValue &coordinateValue : coordinateArray) {
        if ((coordinateValue.type() != QJsonValue::Double) && (coordinateValue.type() != QJsonValue::Null)) {
            errorString =
                QCoreApplication::translate("GeoJsonHelper", "Coordinate array may only contain double values, found: %1").arg(coordinateValue.type());
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
        errorString = QCoreApplication::translate("GeoJsonHelper", "value for coordinate is not array");
        return false;
    }

    const QJsonArray coordinateArray = jsonValue.toArray();
    const int requiredCount = altitudeRequired ? 3 : 2;
    if (coordinateArray.count() != requiredCount) {
        errorString = QCoreApplication::translate("GeoJsonHelper", "Coordinate array must contain %1 values").arg(requiredCount);
        return false;
    }

    for (const QJsonValue &coordinateValue : coordinateArray) {
        if ((coordinateValue.type() != QJsonValue::Double) && (coordinateValue.type() != QJsonValue::Null)) {
            errorString =
                QCoreApplication::translate("GeoJsonHelper", "Coordinate array may only contain double values, found: %1").arg(coordinateValue.type());
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
        errorString = QCoreApplication::translate("GeoJsonHelper", "value for coordinate array is not array");
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
