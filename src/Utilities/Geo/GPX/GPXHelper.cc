#include "GPXHelper.h"
#include "GeoFileIO.h"
#include "GeoUtilities.h"
#include "GPXSchemaValidator.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtPositioning/QGeoPolygon>

QGC_LOGGING_CATEGORY(GPXHelperLog, "Utilities.Geo.GPXHelper")

namespace {
    constexpr const char *kFormatName = "GPX";
    constexpr const char *kGpxNamespace = "http://www.topografix.com/GPX/1/1";
    constexpr const char *kGpxSchemaLocation = "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd";

    void validateSavedFile([[maybe_unused]] const QString &filePath)
    {
#ifndef NDEBUG
        const auto result = GPXSchemaValidator::instance()->validateFile(filePath);
        if (!result.isValid) {
            qCWarning(GPXHelperLog) << "Post-save validation failed for" << filePath;
            for (const QString &error : result.errors) {
                qCWarning(GPXHelperLog) << "  Error:" << error;
            }
        }
        for (const QString &warning : result.warnings) {
            qCDebug(GPXHelperLog) << "  Warning:" << warning;
        }
#endif
    }

    QGeoCoordinate parsePoint(QXmlStreamReader &xml)
    {
        const QXmlStreamAttributes attrs = xml.attributes();
        bool latOk = false, lonOk = false;
        double lat = attrs.value(QStringLiteral("lat")).toDouble(&latOk);
        double lon = attrs.value(QStringLiteral("lon")).toDouble(&lonOk);

        if (!latOk || !lonOk) {
            return QGeoCoordinate();
        }

        // Validate and normalize coordinates
        if (lat < -90.0 || lat > 90.0) {
            qCWarning(GPXHelperLog) << "Latitude out of range, clamping:" << lat;
            lat = GeoUtilities::normalizeLatitude(lat);
        }
        if (lon < -180.0 || lon > 180.0) {
            qCWarning(GPXHelperLog) << "Longitude out of range, wrapping:" << lon;
            lon = GeoUtilities::normalizeLongitude(lon);
        }

        double altitude = qQNaN();

        while (xml.readNextStartElement()) {
            if (xml.name() == QStringLiteral("ele")) {
                bool ok = false;
                const double ele = xml.readElementText().toDouble(&ok);
                if (ok) {
                    // Validate altitude range
                    if (!GeoUtilities::isValidAltitude(ele)) {
                        qCWarning(GPXHelperLog) << "Altitude out of expected range:" << ele;
                    }
                    altitude = ele;
                }
            } else {
                xml.skipCurrentElement();
            }
        }

        if (std::isnan(altitude)) {
            return QGeoCoordinate(lat, lon);
        }
        return QGeoCoordinate(lat, lon, altitude);
    }

    QList<QGeoCoordinate> parseRoute(QXmlStreamReader &xml)
    {
        QList<QGeoCoordinate> coords;

        while (xml.readNextStartElement()) {
            if (xml.name() == QStringLiteral("rtept")) {
                const QGeoCoordinate coord = parsePoint(xml);
                if (coord.isValid()) {
                    coords.append(coord);
                }
            } else {
                xml.skipCurrentElement();
            }
        }

        return coords;
    }

    QList<QGeoCoordinate> parseTrackSegment(QXmlStreamReader &xml)
    {
        QList<QGeoCoordinate> coords;

        while (xml.readNextStartElement()) {
            if (xml.name() == QStringLiteral("trkpt")) {
                const QGeoCoordinate coord = parsePoint(xml);
                if (coord.isValid()) {
                    coords.append(coord);
                }
            } else {
                xml.skipCurrentElement();
            }
        }

        return coords;
    }

    QList<QList<QGeoCoordinate>> parseTrack(QXmlStreamReader &xml)
    {
        QList<QList<QGeoCoordinate>> segments;

        while (xml.readNextStartElement()) {
            if (xml.name() == QStringLiteral("trkseg")) {
                const QList<QGeoCoordinate> segment = parseTrackSegment(xml);
                if (!segment.isEmpty()) {
                    segments.append(segment);
                }
            } else {
                xml.skipCurrentElement();
            }
        }

        return segments;
    }

    bool isClosed(const QList<QGeoCoordinate> &coords)
    {
        if (coords.count() < GeoUtilities::kMinPolygonVertices) {
            return false;
        }
        return coords.first().distanceTo(coords.last()) < GeoUtilities::kPolygonClosureThresholdMeters;
    }

    void writeGpxHeader(QXmlStreamWriter &xml, const QString &creator = QStringLiteral("QGroundControl"))
    {
        xml.writeStartDocument();
        xml.writeStartElement(QStringLiteral("gpx"));
        xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.1"));
        xml.writeAttribute(QStringLiteral("creator"), creator);
        xml.writeDefaultNamespace(QString::fromLatin1(kGpxNamespace));
        xml.writeAttribute(QStringLiteral("xmlns:xsi"), QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"));
        xml.writeAttribute(QStringLiteral("xsi:schemaLocation"), QString::fromLatin1(kGpxSchemaLocation));
    }

    void writePoint(QXmlStreamWriter &xml, const QString &elementName, const QGeoCoordinate &coord, const QString &name = QString())
    {
        xml.writeStartElement(elementName);
        xml.writeAttribute(QStringLiteral("lat"), QString::number(coord.latitude(), 'f', 8));
        xml.writeAttribute(QStringLiteral("lon"), QString::number(coord.longitude(), 'f', 8));

        if (!std::isnan(coord.altitude())) {
            xml.writeTextElement(QStringLiteral("ele"), QString::number(coord.altitude(), 'f', 2));
        }

        if (!name.isEmpty()) {
            xml.writeTextElement(QStringLiteral("name"), name);
        }

        xml.writeEndElement();
    }

    void writeRoute(QXmlStreamWriter &xml, const QList<QGeoCoordinate> &coords, const QString &name = QString())
    {
        xml.writeStartElement(QStringLiteral("rte"));

        if (!name.isEmpty()) {
            xml.writeTextElement(QStringLiteral("name"), name);
        }

        for (const QGeoCoordinate &coord : coords) {
            writePoint(xml, QStringLiteral("rtept"), coord);
        }

        xml.writeEndElement();
    }

    void writeTrack(QXmlStreamWriter &xml, const QList<QGeoCoordinate> &coords, const QString &name = QString())
    {
        xml.writeStartElement(QStringLiteral("trk"));

        if (!name.isEmpty()) {
            xml.writeTextElement(QStringLiteral("name"), name);
        }

        xml.writeStartElement(QStringLiteral("trkseg"));
        for (const QGeoCoordinate &coord : coords) {
            writePoint(xml, QStringLiteral("trkpt"), coord);
        }
        xml.writeEndElement();

        xml.writeEndElement();
    }

    struct GPXData {
        QList<QGeoCoordinate> waypoints;
        QList<QList<QGeoCoordinate>> routes;
        QList<QList<QGeoCoordinate>> trackSegments;
    };

    bool parseGPXFile(const QString &filePath, GPXData &data, QString &errorString)
    {
        auto streamResult = GeoFileIO::openXmlStreamForRead(filePath, QString::fromLatin1(kFormatName));
        if (!streamResult.success) {
            errorString = streamResult.error;
            return false;
        }

        QXmlStreamReader &xml = *streamResult.reader;

        while (!xml.atEnd() && !xml.hasError()) {
            const QXmlStreamReader::TokenType token = xml.readNext();

            if (token == QXmlStreamReader::StartElement) {
                if (xml.name() == QStringLiteral("wpt")) {
                    const QGeoCoordinate coord = parsePoint(xml);
                    if (coord.isValid()) {
                        data.waypoints.append(coord);
                    }
                } else if (xml.name() == QStringLiteral("rte")) {
                    const QList<QGeoCoordinate> route = parseRoute(xml);
                    if (!route.isEmpty()) {
                        data.routes.append(route);
                    }
                } else if (xml.name() == QStringLiteral("trk")) {
                    const QList<QList<QGeoCoordinate>> segments = parseTrack(xml);
                    data.trackSegments.append(segments);
                }
            }
        }

        if (xml.hasError()) {
            errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
                QObject::tr("XML parse error at line %1: %2")
                    .arg(xml.lineNumber())
                    .arg(xml.errorString()));
            return false;
        }

        return true;
    }
}

namespace GPXHelper {

GeoFormatRegistry::ShapeType determineShapeType(const QString &filePath, QString &errorString)
{
    GPXData data;
    if (!parseGPXFile(filePath, data, errorString)) {
        return GeoFormatRegistry::ShapeType::Error;
    }

    for (const QList<QGeoCoordinate> &route : data.routes) {
        if (isClosed(route)) {
            return GeoFormatRegistry::ShapeType::Polygon;
        }
    }

    for (const QList<QGeoCoordinate> &segment : data.trackSegments) {
        if (isClosed(segment)) {
            return GeoFormatRegistry::ShapeType::Polygon;
        }
    }

    if (!data.routes.isEmpty() || !data.trackSegments.isEmpty()) {
        return GeoFormatRegistry::ShapeType::Polyline;
    }

    if (!data.waypoints.isEmpty()) {
        return GeoFormatRegistry::ShapeType::Point;
    }

    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
        QObject::tr("No geometry found in GPX file"));
    return GeoFormatRegistry::ShapeType::Error;
}

int getEntityCount(const QString &filePath, QString &errorString)
{
    GPXData data;
    if (!parseGPXFile(filePath, data, errorString)) {
        return 0;
    }

    return data.waypoints.count() + data.routes.count() + data.trackSegments.count();
}

bool loadPolygonFromFile(const QString &filePath, QList<QGeoCoordinate> &vertices, QString &errorString, double filterMeters)
{
    vertices.clear();

    GPXData data;
    if (!parseGPXFile(filePath, data, errorString)) {
        return false;
    }

    for (const QList<QGeoCoordinate> &route : data.routes) {
        if (isClosed(route)) {
            vertices = route;
            if (!vertices.isEmpty() && vertices.first().distanceTo(vertices.last()) < GeoUtilities::kPolygonClosureThresholdMeters) {
                vertices.removeLast();
            }
            GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolygonVertices);
            return true;
        }
    }

    for (const QList<QGeoCoordinate> &segment : data.trackSegments) {
        if (isClosed(segment)) {
            vertices = segment;
            if (!vertices.isEmpty() && vertices.first().distanceTo(vertices.last()) < GeoUtilities::kPolygonClosureThresholdMeters) {
                vertices.removeLast();
            }
            GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolygonVertices);
            return true;
        }
    }

    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("No closed polygon found in GPX file"));
    return false;
}

bool loadPolygonsFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polygons, QString &errorString, double filterMeters)
{
    polygons.clear();

    GPXData data;
    if (!parseGPXFile(filePath, data, errorString)) {
        return false;
    }

    for (const QList<QGeoCoordinate> &route : data.routes) {
        if (isClosed(route)) {
            QList<QGeoCoordinate> vertices = route;
            if (!vertices.isEmpty() && vertices.first().distanceTo(vertices.last()) < GeoUtilities::kPolygonClosureThresholdMeters) {
                vertices.removeLast();
            }
            GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolygonVertices);
            if (vertices.count() >= GeoUtilities::kMinPolygonVertices) {
                polygons.append(vertices);
            }
        }
    }

    for (const QList<QGeoCoordinate> &segment : data.trackSegments) {
        if (isClosed(segment)) {
            QList<QGeoCoordinate> vertices = segment;
            if (!vertices.isEmpty() && vertices.first().distanceTo(vertices.last()) < GeoUtilities::kPolygonClosureThresholdMeters) {
                vertices.removeLast();
            }
            GeoFormatRegistry::filterVertices(vertices, filterMeters, GeoUtilities::kMinPolygonVertices);
            if (vertices.count() >= GeoUtilities::kMinPolygonVertices) {
                polygons.append(vertices);
            }
        }
    }

    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("No closed polygons found in GPX file"));
        return false;
    }

    return true;
}

bool loadPolygonWithHolesFromFile(const QString &filePath, QGeoPolygon &polygon, QString &errorString)
{
    Q_UNUSED(filePath);
    Q_UNUSED(polygon);
    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
        QObject::tr("GPX format does not support polygons with holes. Use KML, GeoJSON, or Shapefile instead."));
    return false;
}

bool loadPolygonsWithHolesFromFile(const QString &filePath, QList<QGeoPolygon> &polygons, QString &errorString)
{
    Q_UNUSED(filePath);
    Q_UNUSED(polygons);
    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),
        QObject::tr("GPX format does not support polygons with holes. Use KML, GeoJSON, or Shapefile instead."));
    return false;
}

bool loadPolylineFromFile(const QString &filePath, QList<QGeoCoordinate> &coords, QString &errorString, double filterMeters)
{
    coords.clear();

    GPXData data;
    if (!parseGPXFile(filePath, data, errorString)) {
        return false;
    }

    if (!data.routes.isEmpty()) {
        coords = data.routes.first();
        GeoFormatRegistry::filterVertices(coords, filterMeters, GeoUtilities::kMinPolylineVertices);
        return true;
    }

    if (!data.trackSegments.isEmpty()) {
        coords = data.trackSegments.first();
        GeoFormatRegistry::filterVertices(coords, filterMeters, GeoUtilities::kMinPolylineVertices);
        return true;
    }

    errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("No routes or tracks found in GPX file"));
    return false;
}

bool loadPolylinesFromFile(const QString &filePath, QList<QList<QGeoCoordinate>> &polylines, QString &errorString, double filterMeters)
{
    polylines.clear();

    GPXData data;
    if (!parseGPXFile(filePath, data, errorString)) {
        return false;
    }

    for (QList<QGeoCoordinate> route : data.routes) {
        GeoFormatRegistry::filterVertices(route, filterMeters, GeoUtilities::kMinPolylineVertices);
        if (route.count() >= GeoUtilities::kMinPolylineVertices) {
            polylines.append(route);
        }
    }

    for (QList<QGeoCoordinate> segment : data.trackSegments) {
        GeoFormatRegistry::filterVertices(segment, filterMeters, GeoUtilities::kMinPolylineVertices);
        if (segment.count() >= GeoUtilities::kMinPolylineVertices) {
            polylines.append(segment);
        }
    }

    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("No routes or tracks found in GPX file"));
        return false;
    }

    return true;
}

bool loadPointFromFile(const QString &filePath, QGeoCoordinate &point, QString &errorString)
{
    QList<QGeoCoordinate> points;
    if (!loadPointsFromFile(filePath, points, errorString)) {
        return false;
    }
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName), QObject::tr("No waypoints found in GPX file"));
        return false;
    }
    point = points.first();
    return true;
}

bool loadPointsFromFile(const QString &filePath, QList<QGeoCoordinate> &points, QString &errorString)
{
    points.clear();

    GPXData data;
    if (!parseGPXFile(filePath, data, errorString)) {
        return false;
    }

    points = data.waypoints;

    if (points.isEmpty()) {
        errorString = GeoFileIO::formatLoadError(QString::fromLatin1(kFormatName),QObject::tr("No waypoints found in GPX file"));
        return false;
    }

    return true;
}

bool savePolygonToFile(const QString &filePath, const QList<QGeoCoordinate> &vertices, QString &errorString)
{
    if (vertices.count() < GeoUtilities::kMinPolygonVertices) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QObject::tr("Polygon must have at least 3 vertices"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(vertices, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),validationError);
        return false;
    }

    auto streamResult = GeoFileIO::openXmlStreamForWrite(filePath, QString::fromLatin1(kFormatName));
    if (!streamResult.success) {
        errorString = streamResult.error;
        return false;
    }

    QXmlStreamWriter &xml = *streamResult.writer;
    writeGpxHeader(xml);

    QList<QGeoCoordinate> closedVertices = vertices;
    GeoUtilities::ensureClosingVertex(closedVertices);

    writeRoute(xml, closedVertices, QStringLiteral("Polygon"));

    xml.writeEndElement();
    xml.writeEndDocument();
    if (!GeoFileIO::closeXmlStream(streamResult, QString::fromLatin1(kFormatName), errorString)) {
        return false;
    }

    validateSavedFile(filePath);
    return true;
}

bool savePolygonsToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polygons, QString &errorString)
{
    if (polygons.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QObject::tr("No polygons to save"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validatePolygonListCoordinates(polygons, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        return false;
    }

    auto streamResult = GeoFileIO::openXmlStreamForWrite(filePath, QString::fromLatin1(kFormatName));
    if (!streamResult.success) {
        errorString = streamResult.error;
        return false;
    }

    QXmlStreamWriter &xml = *streamResult.writer;
    writeGpxHeader(xml);

    int index = 1;
    for (const QList<QGeoCoordinate> &vertices : polygons) {
        if (vertices.count() < GeoUtilities::kMinPolygonVertices) {
            continue;
        }

        QList<QGeoCoordinate> closedVertices = vertices;
        GeoUtilities::ensureClosingVertex(closedVertices);

        writeRoute(xml, closedVertices, QStringLiteral("Polygon %1").arg(index++));
    }

    xml.writeEndElement();
    xml.writeEndDocument();
    if (!GeoFileIO::closeXmlStream(streamResult, QString::fromLatin1(kFormatName), errorString)) {
        return false;
    }

    validateSavedFile(filePath);
    return true;
}

bool savePolylineToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString)
{
    if (coords.count() < GeoUtilities::kMinPolylineVertices) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), QObject::tr("Polyline must have at least 2 points"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(coords, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        return false;
    }

    auto streamResult = GeoFileIO::openXmlStreamForWrite(filePath, QString::fromLatin1(kFormatName));
    if (!streamResult.success) {
        errorString = streamResult.error;
        return false;
    }

    QXmlStreamWriter &xml = *streamResult.writer;
    writeGpxHeader(xml);

    writeRoute(xml, coords, QStringLiteral("Route"));

    xml.writeEndElement();
    xml.writeEndDocument();
    if (!GeoFileIO::closeXmlStream(streamResult, QString::fromLatin1(kFormatName), errorString)) {
        return false;
    }

    validateSavedFile(filePath);
    return true;
}

bool savePolylinesToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    if (polylines.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),QObject::tr("No polylines to save"));
        return false;
    }

    // Validate all coordinates before saving
    for (int i = 0; i < polylines.size(); i++) {
        QString validationError;
        if (!GeoUtilities::validateCoordinates(polylines[i], validationError)) {
            errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
                QObject::tr("Polyline %1: %2").arg(i + 1).arg(validationError));
            return false;
        }
    }

    auto streamResult = GeoFileIO::openXmlStreamForWrite(filePath, QString::fromLatin1(kFormatName));
    if (!streamResult.success) {
        errorString = streamResult.error;
        return false;
    }

    QXmlStreamWriter &xml = *streamResult.writer;
    writeGpxHeader(xml);

    int index = 1;
    for (const QList<QGeoCoordinate> &coords : polylines) {
        if (coords.count() >= GeoUtilities::kMinPolylineVertices) {
            writeRoute(xml, coords, QStringLiteral("Route %1").arg(index++));
        }
    }

    xml.writeEndElement();
    xml.writeEndDocument();
    if (!GeoFileIO::closeXmlStream(streamResult, QString::fromLatin1(kFormatName), errorString)) {
        return false;
    }

    validateSavedFile(filePath);
    return true;
}

bool savePointToFile(const QString &filePath, const QGeoCoordinate &point, QString &errorString)
{
    return savePointsToFile(filePath, QList<QGeoCoordinate>() << point, errorString);
}

bool savePointsToFile(const QString &filePath, const QList<QGeoCoordinate> &points, QString &errorString)
{
    if (points.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), QObject::tr("No points to save"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(points, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        return false;
    }

    auto streamResult = GeoFileIO::openXmlStreamForWrite(filePath, QString::fromLatin1(kFormatName));
    if (!streamResult.success) {
        errorString = streamResult.error;
        return false;
    }

    QXmlStreamWriter &xml = *streamResult.writer;
    writeGpxHeader(xml);

    int index = 1;
    for (const QGeoCoordinate &point : points) {
        writePoint(xml, QStringLiteral("wpt"), point, QStringLiteral("WPT%1").arg(index++, 3, 10, QLatin1Char('0')));
    }

    xml.writeEndElement();
    xml.writeEndDocument();
    if (!GeoFileIO::closeXmlStream(streamResult, QString::fromLatin1(kFormatName), errorString)) {
        return false;
    }

    validateSavedFile(filePath);
    return true;
}

bool saveTrackToFile(const QString &filePath, const QList<QGeoCoordinate> &coords, QString &errorString)
{
    if (coords.count() < GeoUtilities::kMinPolylineVertices) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), QObject::tr("Track must have at least 2 points"));
        return false;
    }

    // Validate all coordinates before saving
    QString validationError;
    if (!GeoUtilities::validateCoordinates(coords, validationError)) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), validationError);
        return false;
    }

    auto streamResult = GeoFileIO::openXmlStreamForWrite(filePath, QString::fromLatin1(kFormatName));
    if (!streamResult.success) {
        errorString = streamResult.error;
        return false;
    }

    QXmlStreamWriter &xml = *streamResult.writer;
    writeGpxHeader(xml);

    writeTrack(xml, coords, QStringLiteral("Track"));

    xml.writeEndElement();
    xml.writeEndDocument();
    if (!GeoFileIO::closeXmlStream(streamResult, QString::fromLatin1(kFormatName), errorString)) {
        return false;
    }

    validateSavedFile(filePath);
    return true;
}

bool saveTracksToFile(const QString &filePath, const QList<QList<QGeoCoordinate>> &tracks, QString &errorString)
{
    if (tracks.isEmpty()) {
        errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName), QObject::tr("No tracks to save"));
        return false;
    }

    // Validate all coordinates before saving
    for (int i = 0; i < tracks.size(); i++) {
        QString validationError;
        if (!GeoUtilities::validateCoordinates(tracks[i], validationError)) {
            errorString = GeoFileIO::formatSaveError(QString::fromLatin1(kFormatName),
                QObject::tr("Track %1: %2").arg(i + 1).arg(validationError));
            return false;
        }
    }

    auto streamResult = GeoFileIO::openXmlStreamForWrite(filePath, QString::fromLatin1(kFormatName));
    if (!streamResult.success) {
        errorString = streamResult.error;
        return false;
    }

    QXmlStreamWriter &xml = *streamResult.writer;
    writeGpxHeader(xml);

    int index = 1;
    for (const QList<QGeoCoordinate> &coords : tracks) {
        if (coords.count() >= GeoUtilities::kMinPolylineVertices) {
            writeTrack(xml, coords, QStringLiteral("Track %1").arg(index++));
        }
    }

    xml.writeEndElement();
    xml.writeEndDocument();
    if (!GeoFileIO::closeXmlStream(streamResult, QString::fromLatin1(kFormatName), errorString)) {
        return false;
    }

    validateSavedFile(filePath);
    return true;
}

} // namespace GPXHelper
