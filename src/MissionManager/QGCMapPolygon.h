/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "KMLDomDocument.h"

/// The QGCMapPolygon class provides a polygon which can be displayed on a map using a map visuals control.
/// It maintains a representation of the polygon on QVariantList and QmlObjectListModel format.
class QGCMapPolygon : public QObject
{
    Q_OBJECT

public:
    QGCMapPolygon(QObject* parent = nullptr);
    QGCMapPolygon(const QGCMapPolygon& other, QObject* parent = nullptr);

    const QGCMapPolygon& operator=(const QGCMapPolygon& other);

    Q_PROPERTY(int                  count           READ count                                  NOTIFY countChanged)
    Q_PROPERTY(QVariantList         path            READ path                                   NOTIFY pathChanged)
    Q_PROPERTY(double               area            READ area                                   NOTIFY pathChanged)
    Q_PROPERTY(QmlObjectListModel*  pathModel       READ qmlPathModel                           CONSTANT)
    Q_PROPERTY(bool                 dirty           READ dirty          WRITE setDirty          NOTIFY dirtyChanged)
    Q_PROPERTY(QGeoCoordinate       center          READ center         WRITE setCenter         NOTIFY centerChanged)
    Q_PROPERTY(bool                 centerDrag      READ centerDrag     WRITE setCenterDrag     NOTIFY centerDragChanged)
    Q_PROPERTY(bool                 interactive     READ interactive    WRITE setInteractive    NOTIFY interactiveChanged)
    Q_PROPERTY(bool                 isValid         READ isValid                                NOTIFY isValidChanged)
    Q_PROPERTY(bool                 empty           READ empty                                  NOTIFY isEmptyChanged)
    Q_PROPERTY(bool                 traceMode       READ traceMode      WRITE setTraceMode      NOTIFY traceModeChanged)
    Q_PROPERTY(bool                 showAltColor    READ showAltColor   WRITE setShowAltColor   NOTIFY showAltColorChanged)
    Q_PROPERTY(int                  selectedVertex  READ selectedVertex WRITE selectVertex      NOTIFY selectedVertexChanged)

    Q_INVOKABLE void clear(void);
    Q_INVOKABLE void appendVertex(const QGeoCoordinate& coordinate);
    Q_INVOKABLE void removeVertex(int vertexIndex);
    Q_INVOKABLE void appendVertices(const QVariantList& varCoords);

    void appendVertices(const QList<QGeoCoordinate>& coordinates);

    /// Adjust the value for the specified coordinate
    ///     @param vertexIndex Polygon point index to modify (0-based)
    ///     @param coordinate New coordinate for point
    Q_INVOKABLE void adjustVertex(int vertexIndex, const QGeoCoordinate coordinate);

    /// Splits the segment comprised of vertextIndex -> vertexIndex + 1
    Q_INVOKABLE void splitPolygonSegment(int vertexIndex);

    /// Returns true if the specified coordinate is within the polygon
    Q_INVOKABLE bool containsCoordinate(const QGeoCoordinate& coordinate) const;

    /// Offsets the current polygon edges by the specified distance in meters
    Q_INVOKABLE void offset(double distance);

    /// Loads a polygon from a KML/SH{ file
    /// @return true: success
    Q_INVOKABLE bool loadKMLOrSHPFile(const QString& file);

    /// Returns the path in a list of QGeoCoordinate's format
    QList<QGeoCoordinate> coordinateList(void) const;

    /// Returns the QGeoCoordinate for the vertex specified
    Q_INVOKABLE QGeoCoordinate vertexCoordinate(int vertex) const;

    /// Adjust polygon winding order to be clockwise (if needed)
    Q_INVOKABLE void verifyClockwiseWinding(void);

    Q_INVOKABLE void beginReset (void);
    Q_INVOKABLE void endReset   (void);

    /// Saves the polygon to the json object.
    ///     @param json Json object to save to
    void saveToJson(QJsonObject& json);

    /// Load a polygon from json
    ///     @param json Json object to load from
    ///     @param required true: no polygon in object will generate error
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, bool required, QString& errorString);

    /// Convert polygon to NED and return (D is ignored)
    QList<QPointF> nedPolygon(void) const;

    /// Returns the area of the polygon in meters squared
    double area(void) const;

    QDomElement kmlPolygonElement(KMLDomDocument& domDocument);

    // Property methods

    int             count       (void) const { return _polygonPath.count(); }
    bool            dirty       (void) const { return _dirty; }
    void            setDirty    (bool dirty);
    QGeoCoordinate  center      (void) const { return _center; }
    bool            centerDrag  (void) const { return _centerDrag; }
    bool            interactive (void) const { return _interactive; }
    bool            isValid     (void) const { return _polygonModel.count() >= 3; }
    bool            empty       (void) const { return _polygonModel.count() == 0; }
    bool            traceMode   (void) const { return _traceMode; }
    bool            showAltColor(void) const { return _showAltColor; }
    int             selectedVertex()   const { return _selectedVertexIndex; }

    QVariantList        path        (void) const { return _polygonPath; }
    QmlObjectListModel* qmlPathModel(void) { return &_polygonModel; }
    QmlObjectListModel& pathModel   (void) { return _polygonModel; }

    void setPath        (const QList<QGeoCoordinate>& path);
    void setPath        (const QVariantList& path);
    void setCenter      (QGeoCoordinate newCenter);
    void setCenterDrag  (bool centerDrag);
    void setInteractive (bool interactive);
    void setTraceMode   (bool traceMode);
    void setShowAltColor(bool showAltColor);
    void selectVertex   (int index);

    static const char* jsonPolygonKey;

signals:
    void countChanged       (int count);
    void pathChanged        (void);
    void dirtyChanged       (bool dirty);
    void cleared            (void);
    void centerChanged      (QGeoCoordinate center);
    void centerDragChanged  (bool centerDrag);
    void interactiveChanged (bool interactive);
    bool isValidChanged     (void);
    bool isEmptyChanged     (void);
    void traceModeChanged   (bool traceMode);
    void showAltColorChanged(bool showAltColor);
    void selectedVertexChanged(int index);

private slots:
    void _polygonModelCountChanged(int count);
    void _polygonModelDirtyChanged(bool dirty);
    void _updateCenter(void);

private:
    void            _init                   (void);
    QPolygonF       _toPolygonF             (void) const;
    QGeoCoordinate  _coordFromPointF        (const QPointF& point) const;
    QPointF         _pointFFromCoord        (const QGeoCoordinate& coordinate) const;
    void            _beginResetIfNotActive  (void);
    void            _endResetIfNotActive    (void);

    QVariantList        _polygonPath;
    QmlObjectListModel  _polygonModel;
    bool                _dirty =                false;
    QGeoCoordinate      _center;
    bool                _centerDrag =           false;
    bool                _ignoreCenterUpdates =  false;
    bool                _interactive =          false;
    bool                _resetActive =          false;
    bool                _traceMode =            false;
    bool                _showAltColor =         false;
    int                 _selectedVertexIndex =  -1;
};

#endif
