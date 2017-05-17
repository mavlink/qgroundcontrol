/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QGeoCoordinate>
#include <QVariantList>
#include <QPolygon>

#include "QmlObjectListModel.h"

/// The QGCMapCircle represents a circular area which can be displayed on a Map control.
class QGCMapCircle : public QObject
{
    Q_OBJECT

public:
    QGCMapCircle(QObject* parent = NULL);
    QGCMapCircle(const QGeoCoordinate& center, double radius, QObject* parent = NULL);
    QGCMapCircle(const QGCMapCircle& other, QObject* parent = NULL);

    const QGCMapCircle& operator=(const QGCMapCircle& other);

    Q_PROPERTY(bool             dirty   READ dirty  WRITE setDirty  NOTIFY dirtyChanged)
    Q_PROPERTY(QGeoCoordinate   center  READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(double           radius  READ radius WRITE setRadius NOTIFY radiusChanged)

    /// Saves the polygon to the json object.
    ///     @param json Json object to save to
    void saveToJson(QJsonObject& json);

    /// Load a circle from json
    ///     @param json Json object to load from
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, QString& errorString);

    // Property methods

    bool            dirty   (void) const { return _dirty; }
    QGeoCoordinate  center  (void) const { return _center; }
    double          radius  (void) const { return _radius; }

    void setDirty   (bool dirty);
    void setCenter  (QGeoCoordinate newCenter);
    void setRadius  (double radius);

    static const char* jsonCircleKey;

signals:
    void dirtyChanged   (bool dirty);
    void centerChanged  (QGeoCoordinate center);
    void radiusChanged  (double radius);

private slots:
    void _setDirty(void);

private:
    void _init(void);

    bool            _dirty;
    QGeoCoordinate  _center;
    double          _radius;

    static const char* _jsonCenterKey;
    static const char* _jsonRadiusKey;
};
