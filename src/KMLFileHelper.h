/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QDomDocument>
#include <QList>
#include <QGeoCoordinate>

#include "ShapeFileHelper.h"

class KMLFileHelper : public QObject
{
    Q_OBJECT

public:
    static ShapeFileHelper::ShapeType determineShapeType(const QString& kmlFile, QString& errorString);
    static bool loadPolygonFromFile(const QString& kmlFile, QList<QGeoCoordinate>& vertices, QString& errorString);
    static bool loadPolylineFromFile(const QString& kmlFile, QList<QGeoCoordinate>& coords, QString& errorString);

private:
    static QDomDocument _loadFile(const QString& kmlFile, QString& errorString);

    static const char* _errorPrefix;
};
