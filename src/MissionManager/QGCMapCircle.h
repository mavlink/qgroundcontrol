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
#include <QPolygon>

#include "QmlObjectListModel.h"
#include "FactSystem.h"

/// The QGCMapCircle represents a circular area which can be displayed on a Map control.
class QGCMapCircle : public QObject
{
    Q_OBJECT

public:
    QGCMapCircle(QObject* parent = nullptr);
    QGCMapCircle(const QGeoCoordinate& center, double radius, QObject* parent = nullptr);
    QGCMapCircle(const QGeoCoordinate& center, double radius, bool showRotation, bool clockwiseRotation, QObject* parent = nullptr);
    QGCMapCircle(const QGCMapCircle& other, QObject* parent = nullptr);

    const QGCMapCircle& operator=(const QGCMapCircle& other);

    Q_PROPERTY(bool             dirty               READ dirty              WRITE setDirty              NOTIFY dirtyChanged)
    Q_PROPERTY(QGeoCoordinate   center              READ center             WRITE setCenter             NOTIFY centerChanged)
    Q_PROPERTY(Fact*            radius              READ radius                                         CONSTANT)
    Q_PROPERTY(bool             interactive         READ interactive        WRITE setInteractive        NOTIFY interactiveChanged)
    Q_PROPERTY(bool             showRotation        READ showRotation       WRITE setShowRotation       NOTIFY showRotationChanged)
    Q_PROPERTY(bool             clockwiseRotation   READ clockwiseRotation  WRITE setClockwiseRotation  NOTIFY clockwiseRotationChanged)

    /// Saves the polygon to the json object.
    ///     @param json Json object to save to
    void saveToJson(QJsonObject& json);

    /// Load a circle from json
    ///     @param json Json object to load from
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, QString& errorString);

    // Property methods

    bool            dirty               (void) const { return _dirty; }
    QGeoCoordinate  center              (void) const { return _center; }
    Fact*           radius              (void) { return &_radius; }
    bool            interactive         (void) const { return _interactive; }
    bool            showRotation        (void) const { return _showRotation; }
    bool            clockwiseRotation   (void) const { return _clockwiseRotation; }

    void setDirty               (bool dirty);
    void setCenter              (QGeoCoordinate newCenter);
    void setInteractive         (bool interactive);
    void setShowRotation        (bool showRotation);
    void setClockwiseRotation   (bool clockwiseRotation);

    static const char* jsonCircleKey;

signals:
    void dirtyChanged               (bool dirty);
    void centerChanged              (QGeoCoordinate center);
    void interactiveChanged         (bool interactive);
    void showRotationChanged        (bool showRotation);
    void clockwiseRotationChanged   (bool clockwiseRotation);

private slots:
    void _setDirty(void);

private:
    void _init(void);

    bool            _dirty;
    QGeoCoordinate  _center;
    Fact            _radius;
    bool            _interactive;
    bool            _showRotation;
    bool            _clockwiseRotation;

    QMap<QString, FactMetaData*> _nameToMetaDataMap;

    static const char* _jsonCenterKey;
    static const char* _jsonRadiusKey;
    static const char* _radiusFactName;
};
