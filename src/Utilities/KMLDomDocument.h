/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtGui/QColor>

class MissionItem;
class Vehicle;
class QGeoCoordinate;

/// Used to convert a Plan to a KML document
class KMLDomDocument : public QDomDocument
{

public:
    KMLDomDocument(const QString& name);

    void        appendChildToRoot   (const QDomNode& child);
    QDomElement addPlacemark        (const QString& name, bool visible);
    void        addTextElement      (QDomElement& parentElement, const QString& name, const QString& value);
    QString     kmlColorString      (const QColor& color, double opacity = 1);
    QString     kmlCoordString      (const QGeoCoordinate& coord);
    void        addLookAt           (QDomElement& parentElement, const QGeoCoordinate& coord);

    static constexpr const char* balloonStyleName = "BalloonStyle";

protected:
    QDomElement _rootDocumentElement;


private:
    void _addStandardStyles(void);
};
