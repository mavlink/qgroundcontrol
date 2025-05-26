/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "KMLHelper.h"

#include <QtCore/QFile>
#include <QtXml/QDomDocument>

namespace KMLHelper
{
    QDomDocument _loadFile(const QString &kmlFile, QString &errorString);

    constexpr const char *_errorPrefix = QT_TR_NOOP("KML file load failed. %1");
}

QDomDocument KMLHelper::_loadFile(const QString &kmlFile, QString &errorString)
{
    errorString.clear();

    QFile file(kmlFile);
    if (!file.exists()) {
        errorString = QString(_errorPrefix).arg(QString(QT_TRANSLATE_NOOP("KML", "File not found: %1")).arg(kmlFile));
        return QDomDocument();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        errorString = QString(_errorPrefix).arg(QString(QT_TRANSLATE_NOOP("KML", "Unable to open file: %1 error: $%2")).arg(kmlFile).arg(file.errorString()));
        return QDomDocument();
    }

    QDomDocument doc;
    const QDomDocument::ParseResult result = doc.setContent(&file, QDomDocument::ParseOption::Default);
    if (!result) {
        errorString = QString(_errorPrefix).arg(QString(QT_TRANSLATE_NOOP("KML", "Unable to parse KML file: %1 error: %2 line: %3")).arg(kmlFile).arg(result.errorMessage).arg(result.errorLine));
        return QDomDocument();
    }

    return doc;
}

ShapeFileHelper::ShapeType KMLHelper::determineShapeType(const QString &kmlFile, QString &errorString)
{
    using ShapeType = ShapeFileHelper::ShapeType;

    const QDomDocument domDocument = KMLHelper::_loadFile(kmlFile, errorString);
    if (!errorString.isEmpty()) {
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

    errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("KML", "No supported type found in KML file."));
    return ShapeType::Error;
}

bool KMLHelper::loadPolygonFromFile(const QString &kmlFile, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    errorString.clear();
    vertices.clear();

    const QDomDocument domDocument = KMLHelper::_loadFile(kmlFile, errorString);
    if (!errorString.isEmpty()) {
        return false;
    }

    const QDomNodeList rgNodes = domDocument.elementsByTagName("Polygon");
    if (rgNodes.isEmpty()) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("KML", "Unable to find Polygon node in KML"));
        return false;
    }

    const QDomNode coordinatesNode = rgNodes.item(0).namedItem("outerBoundaryIs").namedItem("LinearRing").namedItem("coordinates");
    if (coordinatesNode.isNull()) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("KML", "Internal error: Unable to find coordinates node in KML"));
        return false;
    }

    const QString coordinatesString = coordinatesNode.toElement().text().simplified();
    const QStringList rgCoordinateStrings = coordinatesString.split(" ");

    QList<QGeoCoordinate> rgCoords;
    for (const QString &coordinateString : rgCoordinateStrings) {
        const QStringList rgValueStrings = coordinateString.split(",");
        const QGeoCoordinate coord(rgValueStrings[1].toDouble(), rgValueStrings[0].toDouble());
        rgCoords.append(coord);
    }

    // Determine winding, reverse if needed. QGC wants clockwise winding
    double sum = 0;
    for (int i=0; i<rgCoords.count(); i++) {
        const QGeoCoordinate coord1 = rgCoords[i];
        const QGeoCoordinate coord2 = (i == (rgCoords.count() - 1)) ? rgCoords[0] : rgCoords[i+1];

        sum += (coord2.longitude() - coord1.longitude()) * (coord2.latitude() + coord1.latitude());
    }

    const bool reverse = sum < 0.0;
    if (reverse) {
        QList<QGeoCoordinate> rgReversed;
        for (const QGeoCoordinate &coord : rgCoords) {
            rgReversed.prepend(coord);
        }
        rgCoords = rgReversed;
    }

    vertices = rgCoords;

    return true;
}

bool KMLHelper::loadPolylineFromFile(const QString &kmlFile, QList<QGeoCoordinate> &coords, QString &errorString)
{
    errorString.clear();
    coords.clear();

    const QDomDocument domDocument = KMLHelper::_loadFile(kmlFile, errorString);
    if (!errorString.isEmpty()) {
        return false;
    }

    const QDomNodeList rgNodes = domDocument.elementsByTagName("LineString");
    if (rgNodes.isEmpty()) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("KML", "Unable to find LineString node in KML"));
        return false;
    }

    const QDomNode coordinatesNode = rgNodes.item(0).namedItem("coordinates");
    if (coordinatesNode.isNull()) {
        errorString = QString(_errorPrefix).arg(QT_TRANSLATE_NOOP("KML", "Internal error: Unable to find coordinates node in KML"));
        return false;
    }

    const QString coordinatesString = coordinatesNode.toElement().text().simplified();
    const QStringList rgCoordinateStrings = coordinatesString.split(" ");

    QList<QGeoCoordinate> rgCoords;
    for (const QString &coordinateString : rgCoordinateStrings) {
        const QStringList rgValueStrings = coordinateString.split(",");
        const QGeoCoordinate coord(rgValueStrings[1].toDouble(), rgValueStrings[0].toDouble());
        rgCoords.append(coord);
    }

    coords = rgCoords;

    return true;
}
