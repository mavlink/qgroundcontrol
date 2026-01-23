#include "KMLDomDocument.h"
#include "QGCLoggingCategory.h"

#include <QtGui/QColor>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(KMLDomDocumentLog, "Utilities.Geo.KMLDomDocument")

KMLDomDocument::KMLDomDocument(const QString &name)
{
    const QDomProcessingInstruction header = createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\""));
    (void) appendChild(header);

    QDomElement kmlElement = createElement(QStringLiteral("kml"));
    kmlElement.setAttribute(QStringLiteral("xmlns"), kmlNamespace);
    kmlElement.setAttribute(QStringLiteral("xmlns:xsi"), xsiNamespace);
    kmlElement.setAttribute(QStringLiteral("xsi:schemaLocation"),
                            QStringLiteral("%1 %2").arg(kmlNamespace, kmlSchemaLocation));

    _rootDocumentElement = createElement(QStringLiteral("Document"));
    (void) kmlElement.appendChild(_rootDocumentElement);
    (void) appendChild(kmlElement);

    addTextElement(_rootDocumentElement, "name", name);
    addTextElement(_rootDocumentElement, "open", "1");

    _addStandardStyles();
}

QString KMLDomDocument::kmlCoordString(const QGeoCoordinate &coord)
{
    const double altitude = qIsNaN(coord.altitude()) ? 0 : coord.altitude();
    return QStringLiteral("%1,%2,%3").arg(QString::number(coord.longitude(), 'f', 7), QString::number(coord.latitude(), 'f', 7), QString::number(altitude, 'f', 2));
}

QString KMLDomDocument::kmlColorString(const QColor &color, double opacity)
{
    return QStringLiteral("%1%2%3%4").arg(static_cast<int>(255.0 * opacity), 2, 16, QChar('0')).arg(color.blue(), 2, 16, QChar('0')).arg(color.green(), 2, 16, QChar('0')).arg(color.red(), 2, 16, QChar('0'));
}

void KMLDomDocument::_addStandardStyles()
{
    QDomElement styleElementForBalloon = createElement("Style");
    styleElementForBalloon.setAttribute("id", balloonStyleName);
    QDomElement balloonStyleElement = createElement("BalloonStyle");
    addTextElement(balloonStyleElement, "text", "$[description]");
    (void) styleElementForBalloon.appendChild(balloonStyleElement);
    (void) _rootDocumentElement.appendChild(styleElementForBalloon);
}

void KMLDomDocument::addTextElement(QDomElement &parentElement, const QString &name, const QString &value)
{
    QDomElement textElement = createElement(name);
    (void) textElement.appendChild(createTextNode(value));
    (void) parentElement.appendChild(textElement);
}

void KMLDomDocument::addLookAt(QDomElement &parentElement, const QGeoCoordinate &coord)
{
    QDomElement lookAtElement = createElement("LookAt");
    addTextElement(lookAtElement, "latitude", QString::number(coord.latitude(), 'f', 7));
    addTextElement(lookAtElement, "longitude", QString::number(coord.longitude(), 'f', 7));
    const double altitude = qIsNaN(coord.altitude()) ? 0 : coord.altitude();
    addTextElement(lookAtElement, "altitude", QString::number(altitude, 'f', 2));
    addTextElement(lookAtElement, "heading", "-100");
    addTextElement(lookAtElement, "tilt", "45");
    addTextElement(lookAtElement, "range", "2500");
    (void) parentElement.appendChild(lookAtElement);
}

QDomElement KMLDomDocument::addPlacemark(const QString &name, bool visible)
{
    QDomElement placemarkElement = createElement("Placemark");
    (void) _rootDocumentElement.appendChild(placemarkElement);

    addTextElement(placemarkElement, "name", name);
    addTextElement(placemarkElement, "visibility", visible ? "1" : "0");

    return placemarkElement;
}

void KMLDomDocument::appendChildToRoot(const QDomNode &child)
{
    (void) _rootDocumentElement.appendChild(child);
}

QDomElement KMLDomDocument::addFolder(const QString &name)
{
    QDomElement folderElement = createElement("Folder");
    addTextElement(folderElement, "name", name);
    (void) _rootDocumentElement.appendChild(folderElement);
    return folderElement;
}

void KMLDomDocument::addDescription(QDomElement &parent, const QString &content)
{
    QDomElement descriptionElement = createElement("description");
    QDomCDATASection cdataSection = createCDATASection(content);
    (void) descriptionElement.appendChild(cdataSection);
    (void) parent.appendChild(descriptionElement);
}

QDomElement KMLDomDocument::addStyle(const QString &id)
{
    QDomElement styleElement = createElement("Style");
    styleElement.setAttribute("id", id);
    (void) _rootDocumentElement.appendChild(styleElement);
    return styleElement;
}

void KMLDomDocument::addLineStyle(QDomElement &styleElement, const QColor &color, int width, double opacity)
{
    QDomElement lineStyleElement = createElement("LineStyle");
    addTextElement(lineStyleElement, "color", kmlColorString(color, opacity));
    addTextElement(lineStyleElement, "width", QString::number(width));
    (void) styleElement.appendChild(lineStyleElement);
}

void KMLDomDocument::addPolyStyle(QDomElement &styleElement, const QColor &color, double opacity)
{
    QDomElement polyStyleElement = createElement("PolyStyle");
    addTextElement(polyStyleElement, "color", kmlColorString(color, opacity));
    (void) styleElement.appendChild(polyStyleElement);
}

QDomElement KMLDomDocument::addPoint(QDomElement &parent, const QGeoCoordinate &coord,
                                     const QString &altitudeMode, bool extrude)
{
    QDomElement pointElement = createElement("Point");
    addTextElement(pointElement, "altitudeMode", altitudeMode);
    addTextElement(pointElement, "coordinates", kmlCoordString(coord));
    addTextElement(pointElement, "extrude", extrude ? "1" : "0");
    (void) parent.appendChild(pointElement);
    return pointElement;
}

QDomElement KMLDomDocument::addLineString(QDomElement &parent, const QList<QGeoCoordinate> &coords,
                                          const QString &altitudeMode, bool extrude, bool tessellate)
{
    QDomElement lineStringElement = createElement("LineString");
    addTextElement(lineStringElement, "extrude", extrude ? "1" : "0");
    addTextElement(lineStringElement, "tessellate", tessellate ? "1" : "0");
    addTextElement(lineStringElement, "altitudeMode", altitudeMode);

    QString coordString;
    for (const QGeoCoordinate &coord : coords) {
        coordString += QStringLiteral("%1\n").arg(kmlCoordString(coord));
    }
    addTextElement(lineStringElement, "coordinates", coordString);

    (void) parent.appendChild(lineStringElement);
    return lineStringElement;
}

QDomElement KMLDomDocument::addPolygon(QDomElement &parent, const QList<QGeoCoordinate> &coords,
                                       const QString &altitudeMode)
{
    QDomElement polygonElement = createElement("Polygon");
    addTextElement(polygonElement, "altitudeMode", altitudeMode);

    QDomElement outerBoundaryIs = createElement("outerBoundaryIs");
    QDomElement linearRing = createElement("LinearRing");

    QString coordString;
    for (const QGeoCoordinate &coord : coords) {
        coordString += QStringLiteral("%1\n").arg(kmlCoordString(coord));
    }
    // Close the ring by repeating the first coordinate
    if (!coords.isEmpty()) {
        coordString += QStringLiteral("%1\n").arg(kmlCoordString(coords.first()));
    }
    addTextElement(linearRing, "coordinates", coordString);

    (void) outerBoundaryIs.appendChild(linearRing);
    (void) polygonElement.appendChild(outerBoundaryIs);
    (void) parent.appendChild(polygonElement);
    return polygonElement;
}
