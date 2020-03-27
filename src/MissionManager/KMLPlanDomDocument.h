/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QDomDocument>
#include <QDomElement>
#include <QGeoCoordinate>

class MissionItem;
class Vehicle;

/// Used to convert a Plan to a KML document
class KMLPlanDomDocument : public QDomDocument
{

public:
    KMLPlanDomDocument();

    void addMissionItems(Vehicle* vehicle, QList<MissionItem*> rgMissionItems);

private:
    void    _addStyles      (void);
    QString _kmlColorString (const QColor& color);
    void    _addTextElement (QDomElement& element, const QString& name, const QString& value);
    QString _kmlCoordString (const QGeoCoordinate& coord);
    void    _addLookAt(QDomElement& element, const QGeoCoordinate& coord);

    QDomElement _documentElement;

    static const char* _missionLineStyleName;
    static const char* _ballonStyleName;
};
