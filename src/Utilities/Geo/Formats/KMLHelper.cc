#include "KMLHelper.h"

#include <cmath>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtXml/QDomDocument>

#include "KMLSchemaValidator.h"
#include "QGCLoggingCategory.h"
#include "ShapeVertices.h"

QGC_LOGGING_CATEGORY(KMLHelperLog, "Utilities.KMLHelper")

namespace KMLHelper
{
bool _loadFile(const QString& kmlFile, QDomDocument& doc, QString& errorString);
bool _parseCoordinateString(const QString& coordinatesString, QList<QGeoCoordinate>& coords, QString& errorString);
void _checkAltitudeMode(const QDomNode& geometryNode, const QString& geometryType, int index);
QString _loadError(const QString& detail);

/// Passes the coordinates of each @a tag element of @a kmlFile to @a addShape. An element whose coordinates are
/// missing, cannot be parsed or number fewer than @a minVertices is skipped with a warning.
/// @param outerBoundary the coordinates are those of the element's outer boundary ring, as in a Polygon
/// @param notFoundError translated message for a file without @a tag elements
/// @param noneValidError translated message for a file whose @a tag elements were all skipped
template <typename AddShape>
bool _loadGeometries(const QString& kmlFile, const char* tag, bool outerBoundary, int minVertices,
                     const QString& notFoundError, const QString& noneValidError, QString& errorString,
                     AddShape addShape);
}  // namespace KMLHelper

QString KMLHelper::_loadError(const QString& detail)
{
    return QCoreApplication::translate("KMLHelper", "KML file load failed. %1").arg(detail);
}

bool KMLHelper::_loadFile(const QString& kmlFile, QDomDocument& doc, QString& errorString)
{
    errorString.clear();

    QFile file(kmlFile);
    if (!file.exists()) {
        errorString = _loadError(QCoreApplication::translate("KML", "File not found: %1").arg(kmlFile));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        errorString = _loadError(
            QCoreApplication::translate("KML", "Unable to open file: %1 error: %2").arg(kmlFile, file.errorString()));
        return false;
    }

    const QDomDocument::ParseResult result = doc.setContent(&file, QDomDocument::ParseOption::Default);
    if (!result) {
        errorString = _loadError(QCoreApplication::translate("KML", "Unable to parse KML file: %1 error: %2 line: %3")
                                     .arg(kmlFile)
                                     .arg(result.errorMessage)
                                     .arg(result.errorLine));
        return false;
    }

    return true;
}

bool KMLHelper::_parseCoordinateString(const QString &coordinatesString, QList<QGeoCoordinate> &coords, QString &errorString)
{
    coords.clear();
    const QString simplified = coordinatesString.simplified();
    if (simplified.isEmpty()) {
        errorString = _loadError(QCoreApplication::translate("KML", "Empty coordinates string"));
        return false;
    }

    const QStringList rgCoordinateStrings = simplified.split(' ');
    for (const QString &coordinateString : rgCoordinateStrings) {
        if (coordinateString.isEmpty()) {
            continue;
        }
        const QStringList rgValueStrings = coordinateString.split(',');
        if (rgValueStrings.size() < 2) {
            qCWarning(KMLHelperLog) << "Invalid coordinate format, expected lon,lat[,alt]:" << coordinateString;
            continue;
        }
        bool lonOk = false, latOk = false;
        const double lon = rgValueStrings[0].toDouble(&lonOk);
        const double lat = rgValueStrings[1].toDouble(&latOk);
        // toDouble() reads "nan" and "inf" as numbers
        if (!lonOk || !latOk || !std::isfinite(lon) || !std::isfinite(lat)) {
            qCWarning(KMLHelperLog) << "Failed to parse coordinate values:" << coordinateString;
            continue;
        }
        if (lat < -90.0 || lat > 90.0) {
            qCWarning(KMLHelperLog) << "Latitude out of range [-90, 90]:" << lat << "in:" << coordinateString;
            continue;
        }
        if (lon < -180.0 || lon > 180.0) {
            qCWarning(KMLHelperLog) << "Longitude out of range [-180, 180]:" << lon << "in:" << coordinateString;
            continue;
        }
        double alt = 0.0;
        if (rgValueStrings.size() >= 3) {
            bool altOk = false;
            alt = rgValueStrings[2].toDouble(&altOk);
            // toDouble() reads "nan" and "inf" as numbers, and gives infinity for a number beyond its range
            if (!altOk || !std::isfinite(alt)) {
                alt = 0.0;
                qCWarning(KMLHelperLog) << "Failed to parse altitude, using" << alt << "in:" << coordinateString;
            }
        }
        coords.append(QGeoCoordinate(lat, lon, alt));
    }

    if (coords.isEmpty()) {
        errorString = _loadError(QCoreApplication::translate("KML", "No valid coordinates found"));
        return false;
    }
    return true;
}

void KMLHelper::_checkAltitudeMode(const QDomNode &geometryNode, const QString &geometryType, int index)
{
    // Validate altitudeMode using schema-derived rules
    // QGC treats all coordinates as absolute (AMSL), so warn if a different mode is specified
    const QDomNode altModeNode = geometryNode.namedItem("altitudeMode");
    if (!altModeNode.isNull()) {
        const QString mode = altModeNode.toElement().text();
        if (mode.isEmpty()) {
            return;
        }
        const auto *validator = KMLSchemaValidator::instance();
        const QString location = QStringLiteral("(line %1)").arg(altModeNode.lineNumber());
        if (!validator->isValidEnumValue("altitudeModeEnumType", mode)) {
            qCWarning(KMLHelperLog) << geometryType << index << location << "has invalid altitudeMode:" << mode
                                    << "- valid values are:" << validator->validEnumValues("altitudeModeEnumType").join(", ");
        } else if (mode != "absolute") {
            qCWarning(KMLHelperLog) << geometryType << index << location << "uses altitudeMode:" << mode
                                    << "- QGC will treat coordinates as absolute (AMSL)";
        }
    }
}

ShapeFileHelper::ShapeType KMLHelper::determineShapeType(const QString &kmlFile, QString &errorString)
{
    using ShapeType = ShapeFileHelper::ShapeType;

    QDomDocument domDocument;
    if (!_loadFile(kmlFile, domDocument, errorString)) {
        return ShapeType::Error;
    }

    const QDomNodeList rgNodesPolygon = domDocument.elementsByTagName("Polygon");
    if (!rgNodesPolygon.isEmpty()) {
        return ShapeType::Polygon;
    }

    const QDomNodeList rgNodesLineString = domDocument.elementsByTagName("LineString");
    if (!rgNodesLineString.isEmpty()) {
        return ShapeType::Polyline;
    }

    const QDomNodeList rgNodesPoint = domDocument.elementsByTagName("Point");
    if (!rgNodesPoint.isEmpty()) {
        return ShapeType::Point;
    }

    errorString = _loadError(QCoreApplication::translate("KML", "No supported type found in KML file."));
    return ShapeType::Error;
}

int KMLHelper::getEntityCount(const QString &kmlFile, QString &errorString)
{
    QDomDocument domDocument;
    if (!_loadFile(kmlFile, domDocument, errorString)) {
        return -1;
    }

    int count = 0;
    count += domDocument.elementsByTagName("Polygon").count();
    count += domDocument.elementsByTagName("LineString").count();
    count += domDocument.elementsByTagName("Point").count();
    return count;
}

template <typename AddShape>
bool KMLHelper::_loadGeometries(const QString& kmlFile, const char* tag, bool outerBoundary, int minVertices,
                                const QString& notFoundError, const QString& noneValidError, QString& errorString,
                                AddShape addShape)
{
    QDomDocument domDocument;
    if (!_loadFile(kmlFile, domDocument, errorString)) {
        return false;
    }

    const QDomNodeList rgNodes = domDocument.elementsByTagName(tag);
    if (rgNodes.isEmpty()) {
        errorString = _loadError(notFoundError);
        return false;
    }

    bool anyValid = false;
    for (int nodeIdx = 0; nodeIdx < rgNodes.count(); nodeIdx++) {
        const QDomNode geometryNode = rgNodes.item(nodeIdx);
        _checkAltitudeMode(geometryNode, tag, nodeIdx);

        const QDomNode ringNode =
            outerBoundary ? geometryNode.namedItem("outerBoundaryIs").namedItem("LinearRing") : geometryNode;
        const QDomNode coordinatesNode = ringNode.namedItem("coordinates");
        if (coordinatesNode.isNull()) {
            qCWarning(KMLHelperLog) << tag << nodeIdx << QStringLiteral("(line %1)").arg(geometryNode.lineNumber())
                                    << "missing coordinates node, skipping";
            continue;
        }

        QList<QGeoCoordinate> rgCoords;
        if (!_parseCoordinateString(coordinatesNode.toElement().text(), rgCoords, errorString)) {
            qCWarning(KMLHelperLog) << tag << nodeIdx << QStringLiteral("(line %1)").arg(coordinatesNode.lineNumber())
                                    << "failed to parse coordinates:" << errorString;
            errorString.clear();
            continue;
        }

        if (rgCoords.count() < minVertices) {
            qCWarning(KMLHelperLog) << tag << nodeIdx << QStringLiteral("(line %1)").arg(geometryNode.lineNumber())
                                    << "has fewer than" << minVertices << "vertices, skipping";
            continue;
        }

        addShape(rgCoords);
        anyValid = true;
    }

    if (!anyValid) {
        errorString = _loadError(noneValidError);
        return false;
    }

    return true;
}

bool KMLHelper::loadPolygonsFromFile(const QString& kmlFile, QList<QList<QGeoCoordinate>>& polygons,
                                     QString& errorString, double filterMeters)
{
    polygons.clear();
    return _loadGeometries(kmlFile, "Polygon", true, 3,
                           QCoreApplication::translate("KML", "Unable to find Polygon node in KML"),
                           QCoreApplication::translate("KML", "No valid polygons found in KML file"), errorString,
                           [&polygons, filterMeters](QList<QGeoCoordinate>& vertices) {
                               ShapeVertices::removeClosingVertex(vertices);
                               ShapeVertices::makeClockwise(vertices);
                               ShapeVertices::filterNearby(vertices, filterMeters, 3);
                               polygons.append(vertices);
                           });
}

bool KMLHelper::loadPolylinesFromFile(const QString &kmlFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    polylines.clear();
    return _loadGeometries(kmlFile, "LineString", false, 2,
                           QCoreApplication::translate("KML", "Unable to find LineString node in KML"),
                           QCoreApplication::translate("KML", "No valid polylines found in KML file"), errorString,
                           [&polylines, filterMeters](QList<QGeoCoordinate>& vertices) {
                               ShapeVertices::filterNearby(vertices, filterMeters, 2);
                               polylines.append(vertices);
                           });
}

bool KMLHelper::loadPointsFromFile(const QString &kmlFile, QList<QGeoCoordinate> &points, QString &errorString)
{
    points.clear();
    return _loadGeometries(kmlFile, "Point", false, 1,
                           QCoreApplication::translate("KML", "Unable to find Point node in KML"),
                           QCoreApplication::translate("KML", "No valid points found in KML file"), errorString,
                           [&points](const QList<QGeoCoordinate>& coords) { points.append(coords.first()); });
}
