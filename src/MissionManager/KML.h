/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef KML_H
#define KML_H

#include <QDomDocument>
#include <QDomElement>

class Kml
{

public:
    Kml();
    ~Kml();

    void points(const QStringList& points);
    void polygon(const QStringList& points);
    void save(QDomDocument& document);

private:
    void createHeader();
    void createLookAt(QDomElement& placemark, const QStringList &lookAtList);
    void createStyles();
    void createStyleLine(QDomElement& domEle, const QString& lineColor, const QString& lineWidth, const QString& polyColor);
    void createTextElement(QDomElement& domEle, const QString& elementName, const QString& textElement);

    QDomDocument _domDocument;
    QDomElement _docEle;
    static const QString _encoding;
    static const QString _opengis;
    static const QString _qgckml;
    static const QString _version;
};

#endif
