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

#include "QmlObjectListModel.h"

/// The QGCMapPolygon class provides a polygon which can be displayed on a map using a map visuals control.
/// It maintains a representation of the polygon on QVariantList and QmlObjectListModel format.
class QGCMapPolygon : public QObject
{
    Q_OBJECT

public:
    QGCMapPolygon(QObject* parent = NULL);
    QGCMapPolygon(const QGCMapPolygon& other, QObject* parent = NULL);

    const QGCMapPolygon& operator=(const QGCMapPolygon& other);

    Q_PROPERTY(int                  count       READ count                                  NOTIFY countChanged)
    Q_PROPERTY(QVariantList         path        READ path                                   NOTIFY pathChanged)
    Q_PROPERTY(QmlObjectListModel*  pathModel   READ qmlPathModel                           CONSTANT)
    Q_PROPERTY(bool                 dirty       READ dirty          WRITE setDirty          NOTIFY dirtyChanged)
    Q_PROPERTY(QGeoCoordinate       center      READ center         WRITE setCenter         NOTIFY centerChanged)
    Q_PROPERTY(bool                 centerDrag  READ centerDrag     WRITE setCenterDrag     NOTIFY centerDragChanged)
    Q_PROPERTY(bool                 interactive READ interactive    WRITE setInteractive    NOTIFY interactiveChanged)

    Q_INVOKABLE void clear(void);
    Q_INVOKABLE void appendVertex(const QGeoCoordinate& coordinate);
    Q_INVOKABLE void removeVertex(int vertexIndex);

    /// Adjust the value for the specified coordinate
    ///     @param vertexIndex Polygon point index to modify (0-based)
    ///     @param coordinate New coordinate for point
    Q_INVOKABLE void adjustVertex(int vertexIndex, const QGeoCoordinate coordinate);

    /// Splits the segment comprised of vertextIndex -> vertexIndex + 1
    Q_INVOKABLE void splitPolygonSegment(int vertexIndex);

    /// Returns true if the specified coordinate is within the polygon
    Q_INVOKABLE bool containsCoordinate(const QGeoCoordinate& coordinate) const;

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

    int             count       (void) const { return _polygonPath.count(); }
    bool            dirty       (void) const { return _dirty; }
    void            setDirty    (bool dirty);
    QGeoCoordinate  center      (void) const { return _center; }
    bool            centerDrag  (void) const { return _centerDrag; }
    bool            interactive (void) const { return _interactive; }

    QVariantList        path        (void) const { return _polygonPath; }
    QmlObjectListModel* qmlPathModel(void) { return &_polygonModel; }
    QmlObjectListModel& pathModel   (void) { return _polygonModel; }

    void setPath        (const QList<QGeoCoordinate>& path);
    void setPath        (const QVariantList& path);
    void setCenter      (QGeoCoordinate newCenter);
    void setCenterDrag  (bool centerDrag);
    void setInteractive (bool interactive);

    static const char* jsonPolygonKey;

signals:
    void countChanged       (int count);
    void pathChanged        (void);
    void dirtyChanged       (bool dirty);
    void cleared            (void);
    void centerChanged      (QGeoCoordinate center);
    void centerDragChanged  (bool centerDrag);
    void interactiveChanged (bool interactive);

private slots:
    void _polygonModelCountChanged(int count);
    void _polygonModelDirtyChanged(bool dirty);
    void _updateCenter(void);

private:
    void _init(void);
    QPolygonF _toPolygonF(void) const;
    QGeoCoordinate _coordFromPointF(const QPointF& point) const;
    QPointF _pointFFromCoord(const QGeoCoordinate& coordinate) const;

    QVariantList        _polygonPath;
    QmlObjectListModel  _polygonModel;
    bool                _dirty;
    QGeoCoordinate      _center;
    bool                _centerDrag;
    bool                _ignoreCenterUpdates;
    bool                _interactive;
};

#endif
