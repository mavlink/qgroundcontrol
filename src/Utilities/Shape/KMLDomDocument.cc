/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "KMLDomDocument.h"
#include <QtPositioning/QGeoCoordinate>

KMLDomDocument::KMLDomDocument(const QString &name)
{
    const QDomProcessingInstruction header = createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\""));
    (void) appendChild(header);

    QDomElement kmlElement = createElement(QStringLiteral("kml"));
    kmlElement.setAttribute(QStringLiteral("xmlns"), "http://www.opengis.net/kml/2.2");

    _rootDocumentElement = createElement(QStringLiteral("Document"));
    (void) kmlElement.appendChild(_rootDocumentElement);
    (void) appendChild(kmlElement);

    addTextElement(_rootDocumentElement, "name", name);
    addTextElement(_rootDocumentElement, "open", "1");

    _addStandardStyles();
}

QString KMLDomDocument::kmlCoordString(const QGeoCoordinate &coord)
{
    const double altitude = qIsNaN(coord.altitude() ) ? 0 : coord.altitude();
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
    addTextElement(lookAtElement, "altitude", QString::number(coord.longitude(), 'f', 2));
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
