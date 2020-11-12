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
#include <QGeoCoordinate>
#include <QVariantList>
#include <QColor>

/**
 * @class AirspaceRestriction
 * Base classe for an airspace restriction
 */

class AirspaceRestriction : public QObject
{
    Q_OBJECT
public:
    AirspaceRestriction(QString advisoryID, QColor color, QColor lineColor, float lineWidth, QObject* parent = nullptr);
    Q_PROPERTY(QString  advisoryID  READ advisoryID CONSTANT)
    Q_PROPERTY(QColor   color       READ color      CONSTANT)
    Q_PROPERTY(QColor   lineColor   READ lineColor  CONSTANT)
    Q_PROPERTY(float    lineWidth   READ lineWidth  CONSTANT)
    QString advisoryID  () { return _advisoryID; }
    QColor  color       () { return _color; }
    QColor  lineColor   () { return _lineColor; }
    float   lineWidth   () { return _lineWidth; }
protected:
    QString         _advisoryID;
    QColor          _color;
    QColor          _lineColor;
    float           _lineWidth;
};

/**
 * @class AirspacePolygonRestriction
 * Base classe for an airspace restriction defined by a polygon
 */

class AirspacePolygonRestriction : public AirspaceRestriction
{
    Q_OBJECT
public:
    AirspacePolygonRestriction(const QVariantList& polygon, QString advisoryID, QColor color, QColor lineColor, float lineWidth, QObject* parent = nullptr);
    Q_PROPERTY(QVariantList polygon READ polygon CONSTANT)
    QVariantList polygon() { return _polygon; }
private:
    QVariantList    _polygon;
};

/**
 * @class AirspaceRestriction
 * Base classe for an airspace restriction defined by a circle
 */

class AirspaceCircularRestriction : public AirspaceRestriction
{
    Q_OBJECT
public:
    AirspaceCircularRestriction(const QGeoCoordinate& center, double radius, QString advisoryID, QColor color, QColor lineColor, float lineWidth, QObject* parent = nullptr);
    Q_PROPERTY(QGeoCoordinate   center READ center CONSTANT)
    Q_PROPERTY(double           radius READ radius CONSTANT)
    QGeoCoordinate   center     () { return _center; }
    double           radius     () { return _radius; }
private:
    QGeoCoordinate  _center;
    double          _radius;
};

