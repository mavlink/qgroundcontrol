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

#include "QmlObjectListModel.h"

class QGCMapPolyline : public QObject
{
    Q_OBJECT

public:
    QGCMapPolyline(QObject* parent = NULL);
    QGCMapPolyline(const QGCMapPolyline& other, QObject* parent = NULL);

    const QGCMapPolyline& operator=(const QGCMapPolyline& other);

    Q_PROPERTY(int                  count       READ count                                  NOTIFY countChanged)
    Q_PROPERTY(QVariantList         path        READ path                                   NOTIFY pathChanged)
    Q_PROPERTY(QmlObjectListModel*  pathModel   READ qmlPathModel                           CONSTANT)
    Q_PROPERTY(bool                 dirty       READ dirty          WRITE setDirty          NOTIFY dirtyChanged)
    Q_PROPERTY(bool                 interactive READ interactive    WRITE setInteractive    NOTIFY interactiveChanged)

    Q_INVOKABLE void clear(void);
    Q_INVOKABLE void appendVertex(const QGeoCoordinate& coordinate);
    Q_INVOKABLE void removeVertex(int vertexIndex);
    Q_INVOKABLE void appendVertices(const QList<QGeoCoordinate>& coordinates);

    /// Adjust the value for the specified coordinate
    ///     @param vertexIndex Polygon point index to modify (0-based)
    ///     @param coordinate New coordinate for point
    Q_INVOKABLE void adjustVertex(int vertexIndex, const QGeoCoordinate coordinate);

    /// Splits the line segment comprised of vertexIndex -> vertexIndex + 1
    Q_INVOKABLE void splitSegment(int vertexIndex);

    /// Offsets the current polyline edges by the specified distance in meters
    /// @return Offset set of vertices
    QList<QGeoCoordinate> offsetPolyline(double distance);

    /// Loads a polyline from a KML file
    /// @return true: success
    Q_INVOKABLE bool loadKMLFile(const QString& kmlFile);

    /// Returns the path in a list of QGeoCoordinate's format
    QList<QGeoCoordinate> coordinateList(void) const;

    /// Returns the QGeoCoordinate for the vertex specified
    QGeoCoordinate vertexCoordinate(int vertex) const;

    /// Saves the polyline to the json object.
    ///     @param json Json object to save to
    void saveToJson(QJsonObject& json);

    /// Load a polyline from json
    ///     @param json Json object to load from
    ///     @param required true: no polygon in object will generate error
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, bool required, QString& errorString);

    /// Convert polyline to NED and return (D is ignored)
    QList<QPointF> nedPolyline(void);

    /// Returns the length of the polyline in meters
    double length(void) const;

    // Property methods

    int             count       (void) const { return _polylinePath.count(); }
    bool            dirty       (void) const { return _dirty; }
    void            setDirty    (bool dirty);
    bool            interactive (void) const { return _interactive; }
    QVariantList    path        (void) const { return _polylinePath; }

    QmlObjectListModel* qmlPathModel(void) { return &_polylineModel; }
    QmlObjectListModel& pathModel   (void) { return _polylineModel; }

    void setPath        (const QList<QGeoCoordinate>& path);
    void setPath        (const QVariantList& path);
    void setInteractive (bool interactive);

    static const char* jsonPolylineKey;

signals:
    void countChanged       (int count);
    void pathChanged        (void);
    void dirtyChanged       (bool dirty);
    void cleared            (void);
    void interactiveChanged (bool interactive);

private slots:
    void _polylineModelCountChanged(int count);
    void _polylineModelDirtyChanged(bool dirty);

private:
    void _init(void);
    QGeoCoordinate _coordFromPointF(const QPointF& point) const;
    QPointF _pointFFromCoord(const QGeoCoordinate& coordinate) const;

    QVariantList        _polylinePath;
    QmlObjectListModel  _polylineModel;
    bool                _dirty;
    bool                _interactive;
};
