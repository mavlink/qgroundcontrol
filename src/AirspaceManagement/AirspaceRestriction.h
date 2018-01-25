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

/**
 * @class AirspaceRestriction
 * Base classe for an airspace restriction
 */

class AirspaceRestriction : public QObject
{
    Q_OBJECT
public:
    AirspaceRestriction(QObject* parent = NULL);
};

/**
 * @class AirspacePolygonRestriction
 * Base classe for an airspace restriction defined by a polygon
 */

class AirspacePolygonRestriction : public AirspaceRestriction
{
    Q_OBJECT
public:
    AirspacePolygonRestriction(const QVariantList& polygon, QObject* parent = NULL);

    Q_PROPERTY(QVariantList polygon MEMBER _polygon CONSTANT)

    const QVariantList& getPolygon() const { return _polygon; }

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
    AirspaceCircularRestriction(const QGeoCoordinate& center, double radius, QObject* parent = NULL);

    Q_PROPERTY(QGeoCoordinate   center MEMBER _center CONSTANT)
    Q_PROPERTY(double           radius MEMBER _radius CONSTANT)

private:
    QGeoCoordinate  _center;
    double          _radius;
};

