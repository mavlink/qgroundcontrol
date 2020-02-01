/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "KML.h"

#include <QDomDocument>
#include <QStringList>

const QString Kml::_version("version=\"1.0\"");
const QString Kml::_encoding("encoding=\"UTF-8\"");
const QString Kml::_opengis("http://www.opengis.net/kml/2.2");
const QString Kml::_qgckml("QGC KML");

Kml::Kml()
{
    //create header
    createHeader();
    //name
    createTextElement(_docEle, "name", _qgckml);
    //open
    createTextElement(_docEle, "open", "1");
    //create style
    createStyles();
}

void Kml::points(const QStringList& points)
{
    //create placemark
    QDomElement placemark = _domDocument.createElement("Placemark");
    _docEle.appendChild(placemark);
    createTextElement(placemark, "styleUrl", "yellowLineGreenPoly");
    createTextElement(placemark, "name", "Absolute");
    createTextElement(placemark, "visibility", "0");
    createTextElement(placemark, "description", "Transparent purple line");

    QStringList latLonAlt = points[0].split(",");
    QStringList lookAtList({latLonAlt[0], latLonAlt[1], "0" \
        , "-100", "45", "2500"});
    createLookAt(placemark, lookAtList);

    //Add linestring
    QDomElement lineString = _domDocument.createElement("LineString");
    placemark.appendChild(lineString);

    //extruder
    createTextElement(lineString, "extruder", "1");
    createTextElement(lineString, "tessellate", "1");
    createTextElement(lineString, "altitudeMode", "absolute");
    QString coordinates;
    for(const auto& point : points) {
        coordinates += point + "\n";
    }
    createTextElement(lineString, "coordinates", coordinates);
}

void Kml::save(QDomDocument& document)
{
    document = _domDocument;
}

void Kml::createHeader()
{
    QDomProcessingInstruction header = _domDocument.createProcessingInstruction("xml", _version + " " + _encoding);
    _domDocument.appendChild(header);
    QDomElement kml = _domDocument.createElement("kml");
    kml.setAttribute("xmlns", _opengis);
    _docEle = _domDocument.createElement("Document");
    kml.appendChild(_docEle);
    _domDocument.appendChild(kml);
}

void Kml::createStyles()
{
    QDomElement style = _domDocument.createElement("Style");
    style.setAttribute("id", "yellowLineGreenPoly");
    createStyleLine(style, "7f00ffff", "4", "7f00ff00");
    _docEle.appendChild(style);
}

void Kml::createLookAt(QDomElement& placemark, const QStringList &lookAtList)
{
    QDomElement lookAt = _domDocument.createElement("LookAt");
    placemark.appendChild(lookAt);
    createTextElement(lookAt, "longitude", lookAtList[0]);
    createTextElement(lookAt, "latitude", lookAtList[1]);
    createTextElement(lookAt, "altitude", lookAtList[2]);
    createTextElement(lookAt, "heading", lookAtList[3]);
    createTextElement(lookAt, "tilt", lookAtList[4]);
    createTextElement(lookAt, "range", lookAtList[5]);
}

void Kml::createTextElement(QDomElement& domEle, const QString& elementName, const QString& textElement)
{
    // <elementName>textElement</elementName>
    auto element = _domDocument.createElement(elementName);
    element.appendChild(_domDocument.createTextNode(textElement));
    domEle.appendChild(element);
}

void Kml::createStyleLine(QDomElement& domEle, const QString& lineColor, const QString& lineWidth, const QString& polyColor)
{
    /*
    <LineStyle>
        <color>7f00ffff</color>
        <width>4</width>
    </LineStyle>
    <PolyStyle>
        <color>7f00ff00</color>
    </PolyStyle>
    */
    auto lineStyle = _domDocument.createElement("LineStyle");
    auto polyStyle = _domDocument.createElement("PolyStyle");
    domEle.appendChild(lineStyle);
    domEle.appendChild(polyStyle);
    createTextElement(lineStyle, "color", lineColor);
    createTextElement(lineStyle, "width", lineWidth);
    createTextElement(polyStyle, "color", polyColor);
}

Kml::~Kml()
{
}
