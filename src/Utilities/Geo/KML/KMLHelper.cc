#include "KMLHelper.h"
#include "GeoFileIO.h"
#include "GeoUtilities.h"
#include "KMLSchemaValidator.h"
#include "QGCLoggingCategory.h"

#include <QtXml/QDomDocument>

#include <algorithm>

QGC_LOGGING_CATEGORY(KMLHelperLog, "Utilities.Geo.KMLHelper")

namespace KMLHelper
{
    void _checkAltitudeMode(const QDomNode &geometryNode, const QString &geometryType, int index);
}

void KMLHelper::_checkAltitudeMode(const QDomNode &geometryNode, const QString &geometryType, int index)
{
    // Validate altitudeMode using schema-derived rules
    // QGC treats all coordinates as absolute (AMSL), so warn if a different mode is specified
    const QDomNode altModeNode = geometryNode.namedItem(QStringLiteral("altitudeMode"));
    if (!altModeNode.isNull()) {
        const QString mode = altModeNode.toElement().text();
        if (mode.isEmpty()) {
            return;
        }
        const auto *validator = KMLSchemaValidator::instance();
        const QString location = QStringLiteral("(line %1)").arg(altModeNode.lineNumber());
        if (!validator->isValidEnumValue(QStringLiteral("altitudeModeEnumType"), mode)) {
            qCWarning(KMLHelperLog) << geometryType << index << location << "has invalid altitudeMode:" << mode
                                    << "- valid values are:" << validator->enumValues(QStringLiteral("altitudeModeEnumType")).join(QStringLiteral(", "));
        } else if (mode != QLatin1String("absolute")) {
            qCWarning(KMLHelperLog) << geometryType << index << location << "uses altitudeMode:" << mode
                                    << "- QGC will treat coordinates as absolute (AMSL)";
        }
    }
}

GeoFormatRegistry::ShapeType KMLHelper::determineShapeType(const QString &kmlFile, QString &errorString)
{
    using ShapeType = GeoFormatRegistry::ShapeType;

    const auto result = GeoFileIO::loadDom(kmlFile, QStringLiteral("KML"));
    if (!result.success) {
        errorString = result.error;
        return ShapeType::Error;
    }

    const QDomDocument &domDocument = result.document;
    const QDomNodeList rgNodesPolygon = domDocument.elementsByTagName(QStringLiteral("Polygon"));
    if (!rgNodesPolygon.isEmpty()) {
        return ShapeType::Polygon;
    }

    const QDomNodeList rgNodesLineString = domDocument.elementsByTagName(QStringLiteral("LineString"));
    if (!rgNodesLineString.isEmpty()) {
        return ShapeType::Polyline;
    }

    const QDomNodeList rgNodesPoint = domDocument.elementsByTagName(QStringLiteral("Point"));
    if (!rgNodesPoint.isEmpty()) {
        return ShapeType::Point;
    }

    errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
        QT_TRANSLATE_NOOP("KML", "No supported type found in KML file."));
    return ShapeType::Error;
}

int KMLHelper::getEntityCount(const QString &kmlFile, QString &errorString)
{
    const auto result = GeoFileIO::loadDom(kmlFile, QStringLiteral("KML"));
    if (!result.success) {
        errorString = result.error;
        return 0;
    }

    const QDomDocument &domDocument = result.document;
    int count = 0;
    count += domDocument.elementsByTagName(QStringLiteral("Polygon")).count();
    count += domDocument.elementsByTagName(QStringLiteral("LineString")).count();
    count += domDocument.elementsByTagName(QStringLiteral("Point")).count();
    return count;
}

bool KMLHelper::loadPolygonFromFile(const QString &kmlFile, QList<QGeoCoordinate> &vertices, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polygons;
    if (!loadPolygonsFromFile(kmlFile, polygons, errorString, filterMeters)) {
        return false;
    }
    vertices = polygons.first();
    return true;
}

bool KMLHelper::loadPolygonsFromFile(const QString &kmlFile, QList<QList<QGeoCoordinate>> &polygons, QString &errorString, double filterMeters)
{
    errorString.clear();
    polygons.clear();

    const auto result = GeoFileIO::loadDom(kmlFile, QStringLiteral("KML"));
    if (!result.success) {
        errorString = result.error;
        return false;
    }

    const QDomDocument &domDocument = result.document;
    const QDomNodeList rgNodes = domDocument.elementsByTagName(QStringLiteral("Polygon"));
    if (rgNodes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "Unable to find Polygon node in KML"));
        return false;
    }

    for (int nodeIdx = 0; nodeIdx < rgNodes.count(); nodeIdx++) {
        const QDomNode polygonNode = rgNodes.item(nodeIdx);
        _checkAltitudeMode(polygonNode, QStringLiteral("Polygon"), nodeIdx);

        // Note: This function ignores holes. Use loadPolygonsWithHolesFromFile() to preserve hole information.
        const QDomNode coordinatesNode = polygonNode.namedItem(QStringLiteral("outerBoundaryIs"))
                                                    .namedItem(QStringLiteral("LinearRing"))
                                                    .namedItem(QStringLiteral("coordinates"));
        if (coordinatesNode.isNull()) {
            qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << QStringLiteral("(line %1)").arg(polygonNode.lineNumber())
                                    << "missing coordinates node, skipping";
            continue;
        }

        QList<QGeoCoordinate> rgCoords;
        QString parseError;
        if (!GeoUtilities::parseKmlCoordinateList(coordinatesNode.toElement().text(), rgCoords, parseError)) {
            qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << QStringLiteral("(line %1)").arg(coordinatesNode.lineNumber())
                                    << "failed to parse coordinates:" << parseError;
            continue;
        }

        if (rgCoords.count() < GeoUtilities::kMinPolygonVertices) {
            qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << QStringLiteral("(line %1)").arg(polygonNode.lineNumber())
                                    << "has fewer than 3 vertices, skipping";
            continue;
        }

        // Remove duplicate closing vertex (KML polygons repeat first vertex at end)
        GeoUtilities::removeClosingVertex(rgCoords);

        // Ensure clockwise winding (QGC convention)
        GeoUtilities::ensureClockwise(rgCoords);

        GeoFormatRegistry::filterVertices(rgCoords, filterMeters, GeoUtilities::kMinPolygonVertices);
        polygons.append(rgCoords);
    }

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "No valid polygons found in KML file"));
        return false;
    }

    return true;
}

bool KMLHelper::loadPolygonWithHolesFromFile(const QString &kmlFile, QGeoPolygon &polygon, QString &errorString)
{
    QList<QGeoPolygon> polygons;
    if (!loadPolygonsWithHolesFromFile(kmlFile, polygons, errorString)) {
        return false;
    }
    polygon = polygons.first();
    return true;
}

bool KMLHelper::loadPolygonsWithHolesFromFile(const QString &kmlFile, QList<QGeoPolygon> &polygons, QString &errorString)
{
    errorString.clear();
    polygons.clear();

    const auto result = GeoFileIO::loadDom(kmlFile, QStringLiteral("KML"));
    if (!result.success) {
        errorString = result.error;
        return false;
    }

    const QDomDocument &domDocument = result.document;
    const QDomNodeList rgNodes = domDocument.elementsByTagName(QStringLiteral("Polygon"));
    if (rgNodes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "Unable to find Polygon node in KML"));
        return false;
    }

    for (int nodeIdx = 0; nodeIdx < rgNodes.count(); nodeIdx++) {
        const QDomNode polygonNode = rgNodes.item(nodeIdx);
        _checkAltitudeMode(polygonNode, QStringLiteral("Polygon"), nodeIdx);

        // Parse outer boundary
        const QDomNode outerCoordinatesNode = polygonNode.namedItem(QStringLiteral("outerBoundaryIs"))
                                                         .namedItem(QStringLiteral("LinearRing"))
                                                         .namedItem(QStringLiteral("coordinates"));
        if (outerCoordinatesNode.isNull()) {
            qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << QStringLiteral("(line %1)").arg(polygonNode.lineNumber())
                                    << "missing outer boundary coordinates, skipping";
            continue;
        }

        QList<QGeoCoordinate> outerRing;
        QString parseError;
        if (!GeoUtilities::parseKmlCoordinateList(outerCoordinatesNode.toElement().text(), outerRing, parseError)) {
            qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << "failed to parse outer boundary:" << parseError;
            continue;
        }

        if (outerRing.count() < GeoUtilities::kMinPolygonVertices) {
            qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << "outer boundary has fewer than 3 vertices, skipping";
            continue;
        }

        // Remove duplicate closing vertex
        GeoUtilities::removeClosingVertex(outerRing);

        // Ensure clockwise winding for outer ring
        GeoUtilities::ensureClockwise(outerRing);

        QGeoPolygon geoPolygon(outerRing);

        // Parse inner boundaries (holes)
        const QDomNodeList innerBoundaries = polygonNode.toElement().elementsByTagName(QStringLiteral("innerBoundaryIs"));
        for (int holeIdx = 0; holeIdx < innerBoundaries.count(); holeIdx++) {
            const QDomNode innerNode = innerBoundaries.item(holeIdx);
            const QDomNode innerCoordinatesNode = innerNode.namedItem(QStringLiteral("LinearRing"))
                                                           .namedItem(QStringLiteral("coordinates"));
            if (innerCoordinatesNode.isNull()) {
                qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << "hole" << holeIdx << "missing coordinates, skipping hole";
                continue;
            }

            QList<QGeoCoordinate> holeRing;
            if (!GeoUtilities::parseKmlCoordinateList(innerCoordinatesNode.toElement().text(), holeRing, parseError)) {
                qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << "hole" << holeIdx << "failed to parse:" << parseError;
                continue;
            }

            if (holeRing.count() < GeoUtilities::kMinPolygonVertices) {
                qCWarning(KMLHelperLog) << "Polygon" << nodeIdx << "hole" << holeIdx << "has fewer than 3 vertices, skipping";
                continue;
            }

            // Remove duplicate closing vertex
            GeoUtilities::removeClosingVertex(holeRing);

            geoPolygon.addHole(holeRing);
        }

        polygons.append(geoPolygon);
    }

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "No valid polygons found in KML file"));
        return false;
    }

    return true;
}

bool KMLHelper::loadPolylineFromFile(const QString &kmlFile, QList<QGeoCoordinate> &coords, QString &errorString, double filterMeters)
{
    QList<QList<QGeoCoordinate>> polylines;
    if (!loadPolylinesFromFile(kmlFile, polylines, errorString, filterMeters)) {
        return false;
    }
    coords = polylines.first();
    return true;
}

bool KMLHelper::loadPolylinesFromFile(const QString &kmlFile, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    errorString.clear();
    polylines.clear();

    const auto result = GeoFileIO::loadDom(kmlFile, QStringLiteral("KML"));
    if (!result.success) {
        errorString = result.error;
        return false;
    }

    const QDomDocument &domDocument = result.document;
    const QDomNodeList rgNodes = domDocument.elementsByTagName(QStringLiteral("LineString"));
    if (rgNodes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "Unable to find LineString node in KML"));
        return false;
    }

    for (int nodeIdx = 0; nodeIdx < rgNodes.count(); nodeIdx++) {
        const QDomNode lineStringNode = rgNodes.item(nodeIdx);
        _checkAltitudeMode(lineStringNode, QStringLiteral("LineString"), nodeIdx);

        const QDomNode coordinatesNode = lineStringNode.namedItem(QStringLiteral("coordinates"));
        if (coordinatesNode.isNull()) {
            qCWarning(KMLHelperLog) << "LineString" << nodeIdx << QStringLiteral("(line %1)").arg(lineStringNode.lineNumber())
                                    << "missing coordinates node, skipping";
            continue;
        }

        QList<QGeoCoordinate> rgCoords;
        QString parseError;
        if (!GeoUtilities::parseKmlCoordinateList(coordinatesNode.toElement().text(), rgCoords, parseError)) {
            qCWarning(KMLHelperLog) << "LineString" << nodeIdx << QStringLiteral("(line %1)").arg(coordinatesNode.lineNumber())
                                    << "failed to parse coordinates:" << parseError;
            continue;
        }

        if (rgCoords.count() < GeoUtilities::kMinPolylineVertices) {
            qCWarning(KMLHelperLog) << "LineString" << nodeIdx << QStringLiteral("(line %1)").arg(lineStringNode.lineNumber())
                                    << "has fewer than 2 vertices, skipping";
            continue;
        }

        GeoFormatRegistry::filterVertices(rgCoords, filterMeters, GeoUtilities::kMinPolylineVertices);
        polylines.append(rgCoords);
    }

    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "No valid polylines found in KML file"));
        return false;
    }

    return true;
}

bool KMLHelper::loadPointFromFile(const QString &kmlFile, QGeoCoordinate &point, QString &errorString)
{
    QList<QGeoCoordinate> points;
    if (!loadPointsFromFile(kmlFile, points, errorString)) {
        return false;
    }
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "No points found in KML file"));
        return false;
    }
    point = points.first();
    return true;
}

bool KMLHelper::loadPointsFromFile(const QString &kmlFile, QList<QGeoCoordinate> &points, QString &errorString)
{
    errorString.clear();
    points.clear();

    const auto result = GeoFileIO::loadDom(kmlFile, QStringLiteral("KML"));
    if (!result.success) {
        errorString = result.error;
        return false;
    }

    const QDomDocument &domDocument = result.document;
    const QDomNodeList rgNodes = domDocument.elementsByTagName(QStringLiteral("Point"));
    if (rgNodes.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "Unable to find Point node in KML"));
        return false;
    }

    for (int nodeIdx = 0; nodeIdx < rgNodes.count(); nodeIdx++) {
        const QDomNode pointNode = rgNodes.item(nodeIdx);
        _checkAltitudeMode(pointNode, QStringLiteral("Point"), nodeIdx);

        const QDomNode coordinatesNode = pointNode.namedItem(QStringLiteral("coordinates"));
        if (coordinatesNode.isNull()) {
            qCWarning(KMLHelperLog) << "Point" << nodeIdx << QStringLiteral("(line %1)").arg(pointNode.lineNumber())
                                    << "missing coordinates node, skipping";
            continue;
        }

        QList<QGeoCoordinate> coords;
        QString parseError;
        if (!GeoUtilities::parseKmlCoordinateList(coordinatesNode.toElement().text(), coords, parseError)) {
            qCWarning(KMLHelperLog) << "Point" << nodeIdx << QStringLiteral("(line %1)").arg(coordinatesNode.lineNumber())
                                    << "failed to parse coordinates:" << parseError;
            continue;
        }

        if (!coords.isEmpty()) {
            points.append(coords.first());
        }
    }

    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QStringLiteral("KML"),
            QT_TRANSLATE_NOOP("KML", "No valid points found in KML file"));
        return false;
    }

    return true;
}

// ============================================================================
// Save functions
// ============================================================================

namespace KMLHelper
{
    constexpr const char *kmlNamespace = "http://www.opengis.net/kml/2.2";

    QDomDocument _createKmlDocument()
    {
        QDomDocument doc;
        doc.appendChild(doc.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));
        QDomElement kml = doc.createElement(QStringLiteral("kml"));
        kml.setAttribute(QStringLiteral("xmlns"), QString::fromLatin1(kmlNamespace));
        doc.appendChild(kml);
        QDomElement document = doc.createElement(QStringLiteral("Document"));
        kml.appendChild(document);
        return doc;
    }

    QDomElement _getDocumentElement(QDomDocument &doc)
    {
        return doc.documentElement().firstChildElement(QStringLiteral("Document"));
    }

    void _addAltitudeMode(QDomDocument &doc, QDomElement &parent, bool hasAltitude)
    {
        if (hasAltitude) {
            QDomElement altMode = doc.createElement(QStringLiteral("altitudeMode"));
            altMode.appendChild(doc.createTextNode(QStringLiteral("absolute")));
            parent.appendChild(altMode);
        }
    }

    bool _saveAndValidate(const QDomDocument &doc, const QString &kmlFile, QString &errorString)
    {
        if (!GeoFileIO::saveDom(kmlFile, doc, QStringLiteral("KML"), errorString)) {
            return false;
        }

        // Validate the saved file in debug builds
#ifndef NDEBUG
        const auto result = KMLSchemaValidator::instance()->validateFile(kmlFile);
        if (!result.isValid) {
            qCWarning(KMLHelperLog) << "Post-save validation failed for" << kmlFile;
            for (const QString &error : result.errors) {
                qCWarning(KMLHelperLog) << "  Error:" << error;
            }
        }
        for (const QString &warning : result.warnings) {
            qCDebug(KMLHelperLog) << "  Warning:" << warning;
        }
#endif

        return true;
    }
}

bool KMLHelper::savePolygonToFile(const QString &kmlFile, const QList<QGeoCoordinate> &vertices, QString &errorString)
{
    return savePolygonsToFile(kmlFile, {vertices}, errorString);
}

bool KMLHelper::savePolygonsToFile(const QString &kmlFile, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString)
{
    errorString.clear();

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"), QObject::tr("No polygons to save"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validatePolygonListCoordinates(polygons, validationError)) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"), validationError);
        return false;
    }

    QDomDocument doc = _createKmlDocument();
    QDomElement document = _getDocumentElement(doc);

    for (int i = 0; i < polygons.count(); i++) {
        const QList<QGeoCoordinate> &vertices = polygons[i];
        if (vertices.count() < GeoUtilities::kMinPolygonVertices) {
            qCWarning(KMLHelperLog) << "Skipping polygon" << i << "with fewer than 3 vertices";
            continue;
        }

        QDomElement placemark = doc.createElement(QStringLiteral("Placemark"));
        document.appendChild(placemark);

        QDomElement polygon = doc.createElement(QStringLiteral("Polygon"));
        placemark.appendChild(polygon);

        const bool hasAltitude = GeoUtilities::hasAnyAltitude(vertices);
        _addAltitudeMode(doc, polygon, hasAltitude);

        QDomElement outerBoundary = doc.createElement(QStringLiteral("outerBoundaryIs"));
        polygon.appendChild(outerBoundary);

        QDomElement linearRing = doc.createElement(QStringLiteral("LinearRing"));
        outerBoundary.appendChild(linearRing);

        // Close the ring (KML requires first == last for polygons)
        QList<QGeoCoordinate> closedVertices = vertices;
        GeoUtilities::ensureClosingVertex(closedVertices);

        QDomElement coordinates = doc.createElement(QStringLiteral("coordinates"));
        coordinates.appendChild(doc.createTextNode(GeoUtilities::formatKmlCoordinateList(closedVertices)));
        linearRing.appendChild(coordinates);
    }

    return _saveAndValidate(doc, kmlFile, errorString);
}

bool KMLHelper::savePolygonWithHolesToFile(const QString &kmlFile, const QGeoPolygon &polygon, QString &errorString)
{
    return savePolygonsWithHolesToFile(kmlFile, {polygon}, errorString);
}

bool KMLHelper::savePolygonsWithHolesToFile(const QString &kmlFile, const QList<QGeoPolygon> &polygons, QString &errorString)
{
    errorString.clear();

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"), QObject::tr("No polygons to save"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateGeoPolygonListCoordinates(polygons, validationError)) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"), validationError);
        return false;
    }

    QDomDocument doc = _createKmlDocument();
    QDomElement document = _getDocumentElement(doc);

    for (int i = 0; i < polygons.count(); i++) {
        const QGeoPolygon &geoPolygon = polygons[i];
        const QList<QGeoCoordinate> perimeter = geoPolygon.perimeter();

        if (perimeter.count() < GeoUtilities::kMinPolygonVertices) {
            qCWarning(KMLHelperLog) << "Skipping polygon" << i << "with fewer than 3 vertices";
            continue;
        }

        QDomElement placemark = doc.createElement(QStringLiteral("Placemark"));
        document.appendChild(placemark);

        QDomElement polygon = doc.createElement(QStringLiteral("Polygon"));
        placemark.appendChild(polygon);

        // Check if any coordinate in outer ring or holes has altitude
        bool hasAltitude = GeoUtilities::hasAnyAltitude(perimeter);
        for (int h = 0; !hasAltitude && h < geoPolygon.holesCount(); h++) {
            hasAltitude = GeoUtilities::hasAnyAltitude(geoPolygon.holePath(h));
        }
        _addAltitudeMode(doc, polygon, hasAltitude);

        // Outer boundary
        QDomElement outerBoundary = doc.createElement(QStringLiteral("outerBoundaryIs"));
        polygon.appendChild(outerBoundary);

        QDomElement outerLinearRing = doc.createElement(QStringLiteral("LinearRing"));
        outerBoundary.appendChild(outerLinearRing);

        // Close the ring
        QList<QGeoCoordinate> closedPerimeter = perimeter;
        if (closedPerimeter.first().latitude() != closedPerimeter.last().latitude() ||
            closedPerimeter.first().longitude() != closedPerimeter.last().longitude()) {
            closedPerimeter.append(closedPerimeter.first());
        }

        QDomElement outerCoordinates = doc.createElement(QStringLiteral("coordinates"));
        outerCoordinates.appendChild(doc.createTextNode(GeoUtilities::formatKmlCoordinateList(closedPerimeter)));
        outerLinearRing.appendChild(outerCoordinates);

        // Inner boundaries (holes)
        for (int h = 0; h < geoPolygon.holesCount(); h++) {
            const QList<QGeoCoordinate> holePath = geoPolygon.holePath(h);

            QDomElement innerBoundary = doc.createElement(QStringLiteral("innerBoundaryIs"));
            polygon.appendChild(innerBoundary);

            QDomElement innerLinearRing = doc.createElement(QStringLiteral("LinearRing"));
            innerBoundary.appendChild(innerLinearRing);

            // Close the ring
            QList<QGeoCoordinate> closedHole = holePath;
            if (closedHole.first().latitude() != closedHole.last().latitude() ||
                closedHole.first().longitude() != closedHole.last().longitude()) {
                closedHole.append(closedHole.first());
            }

            QDomElement innerCoordinates = doc.createElement(QStringLiteral("coordinates"));
            innerCoordinates.appendChild(doc.createTextNode(GeoUtilities::formatKmlCoordinateList(closedHole)));
            innerLinearRing.appendChild(innerCoordinates);
        }
    }

    return _saveAndValidate(doc, kmlFile, errorString);
}

bool KMLHelper::savePolylineToFile(const QString &kmlFile, const QList<QGeoCoordinate> &coords, QString &errorString)
{
    return savePolylinesToFile(kmlFile, {coords}, errorString);
}

bool KMLHelper::savePolylinesToFile(const QString &kmlFile, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    errorString.clear();

    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"), QObject::tr("No polylines to save"));
        return false;
    }

    // Validate all coordinates before saving
    for (int i = 0; i < polylines.size(); i++) {
        QString validationError;
        if (!GeoUtilities::validateCoordinates(polylines[i], validationError)) {
            errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"),
                QObject::tr("Polyline %1: %2").arg(i + 1).arg(validationError));
            return false;
        }
    }

    QDomDocument doc = _createKmlDocument();
    QDomElement document = _getDocumentElement(doc);

    for (int i = 0; i < polylines.count(); i++) {
        const QList<QGeoCoordinate> &coords = polylines[i];
        if (coords.count() < GeoUtilities::kMinPolylineVertices) {
            qCWarning(KMLHelperLog) << "Skipping polyline" << i << "with fewer than 2 vertices";
            continue;
        }

        QDomElement placemark = doc.createElement(QStringLiteral("Placemark"));
        document.appendChild(placemark);

        QDomElement lineString = doc.createElement(QStringLiteral("LineString"));
        placemark.appendChild(lineString);

        const bool hasAltitude = GeoUtilities::hasAnyAltitude(coords);
        _addAltitudeMode(doc, lineString, hasAltitude);

        QDomElement coordinates = doc.createElement(QStringLiteral("coordinates"));
        coordinates.appendChild(doc.createTextNode(GeoUtilities::formatKmlCoordinateList(coords)));
        lineString.appendChild(coordinates);
    }

    return _saveAndValidate(doc, kmlFile, errorString);
}

bool KMLHelper::savePointToFile(const QString &kmlFile, const QGeoCoordinate &point, QString &errorString)
{
    return savePointsToFile(kmlFile, QList<QGeoCoordinate>() << point, errorString);
}

bool KMLHelper::savePointsToFile(const QString &kmlFile, const QList<QGeoCoordinate> &points, QString &errorString)
{
    errorString.clear();

    if (points.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"), QObject::tr("No points to save"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(points, validationError)) {
        errorString = GeoFileIO::formatSaveError(QStringLiteral("KML"), validationError);
        return false;
    }

    QDomDocument doc = _createKmlDocument();
    QDomElement document = _getDocumentElement(doc);

    for (int i = 0; i < points.count(); i++) {
        const QGeoCoordinate &point = points[i];
        if (!point.isValid()) {
            qCWarning(KMLHelperLog) << "Skipping invalid point" << i;
            continue;
        }

        QDomElement placemark = doc.createElement(QStringLiteral("Placemark"));
        document.appendChild(placemark);

        QDomElement pointElem = doc.createElement(QStringLiteral("Point"));
        placemark.appendChild(pointElem);

        const bool hasAltitude = !qIsNaN(point.altitude());
        _addAltitudeMode(doc, pointElem, hasAltitude);

        QDomElement coordinates = doc.createElement(QStringLiteral("coordinates"));
        coordinates.appendChild(doc.createTextNode(GeoUtilities::formatKmlCoordinate(point)));
        pointElem.appendChild(coordinates);
    }

    return _saveAndValidate(doc, kmlFile, errorString);
}
