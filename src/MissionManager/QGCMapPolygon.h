/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef QGCMapPolygon_H
#define QGCMapPolygon_H

#include <QObject>
#include <QGeoCoordinate>
#include <QVariantList>
#include <QPolygon>

/// The QGCMapPolygon class provides a polygon which can be displayed on a map using a MapPolygon control.
/// It works in conjunction with the QGCMapPolygonControls control which provides the UI for drawing and
/// editing map polygons.
class QGCMapPolygon : public QObject
{
    Q_OBJECT

public:
    QGCMapPolygon(QObject* parent = NULL);

    const QGCMapPolygon& operator=(const QGCMapPolygon& other);

    QGeoCoordinate operator[](int index) const { return _polygonPath[index].value<QGeoCoordinate>(); }

    /// The polygon path to be bound to the MapPolygon.path property
    Q_PROPERTY(QVariantList path READ path WRITE setPath NOTIFY pathChanged)

    /// true: Polygon has changed since last time dirty was false
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)

    /// Remove all points from polygon
    Q_INVOKABLE void clear(void);

    /// Adjust the value for the specified coordinate
    ///     @param vertexIndex Polygon point index to modify (0-based)
    ///     @param coordinate New coordinate for point
    Q_INVOKABLE void adjustCoordinate(int vertexIndex, const QGeoCoordinate coordinate);

    /// Returns the center point coordinate for the polygon
    Q_INVOKABLE QGeoCoordinate center(void) const;

    /// Returns true if the specified coordinate is within the polygon
    Q_INVOKABLE bool containsCoordinate(const QGeoCoordinate& coordinate) const;

    /// Returns the number of points in the polygon
    Q_INVOKABLE int count(void) const { return _polygonPath.count(); }

    /// Returns the path in a list of QGeoCoordinate's format
    QList<QGeoCoordinate> coordinateList(void) const;

    /// Saves the polygon to the json object.
    ///     @param json Json object to save to
    void saveToJson(QJsonObject& json);

    /// Load a polygon from json
    ///     @param json Json object to load from
    ///     @param required true: no polygon in object will generate error
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, bool required, QString& errorString);

    // Property methods

    bool dirty(void) const { return _dirty; }
    void setDirty(bool dirty);

    QVariantList path(void) const { return _polygonPath; }
    void setPath(const QList<QGeoCoordinate>& path);
    void setPath(const QVariantList& path);

signals:
    void pathChanged(void);
    void dirtyChanged(bool dirty);

private:
    QPolygonF _toPolygonF(void) const;
    QGeoCoordinate _coordFromPointF(const QPointF& point) const;
    QPointF _pointFFromCoord(const QGeoCoordinate& coordinate) const;

    QVariantList    _polygonPath;
    bool            _dirty;

    static const char* _jsonPolygonKey;
};

#endif
