/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    AirspaceRestriction(QColor color, QObject* parent = nullptr);
    Q_PROPERTY(QColor color  READ color CONSTANT)
    QColor color() { return _color; }
protected:
    QColor         _color;
};

/**
 * @class AirspacePolygonRestriction
 * Base classe for an airspace restriction defined by a polygon
 */

class AirspacePolygonRestriction : public AirspaceRestriction
{
    Q_OBJECT
public:
    AirspacePolygonRestriction(const QVariantList& polygon, QColor color, QObject* parent = nullptr);
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
    AirspaceCircularRestriction(const QGeoCoordinate& center, double radius, QColor color, QObject* parent = nullptr);
    Q_PROPERTY(QGeoCoordinate   center READ center CONSTANT)
    Q_PROPERTY(double           radius READ radius CONSTANT)
    QGeoCoordinate   center     () { return _center; }
    double           radius     () { return _radius; }
private:
    QGeoCoordinate  _center;
    double          _radius;
};

