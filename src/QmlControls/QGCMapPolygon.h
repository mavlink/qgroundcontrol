#pragma once

#include <QtCore/QVariantList>
#include <QtGui/QPolygonF>

#include "QGCMapPathBase.h"

class QGCMapPolygon : public QGCMapPathBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(double area READ area NOTIFY pathChanged)
    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(bool centerDrag READ centerDrag WRITE setCenterDrag NOTIFY centerDragChanged)
    Q_PROPERTY(bool showAltColor READ showAltColor WRITE setShowAltColor NOTIFY showAltColorChanged)
public:
    explicit QGCMapPolygon(QObject* parent = nullptr);
    QGCMapPolygon(const QGCMapPolygon& other, QObject* parent = nullptr);

    ~QGCMapPolygon() override = default;

    const QGCMapPolygon& operator=(const QGCMapPolygon& other);

    using QGCMapPathBase::appendVertices;
    Q_INVOKABLE void appendVertices(const QVariantList& varCoords);

    Q_INVOKABLE void adjustVertex(int vertexIndex, const QGeoCoordinate coordinate) override;
    Q_INVOKABLE void splitPolygonSegment(int vertexIndex);
    Q_INVOKABLE bool containsCoordinate(const QGeoCoordinate& coordinate) const;
    Q_INVOKABLE void offset(double distance);
    Q_INVOKABLE void verifyClockwiseWinding();

    QList<QPointF> nedPolygon() const { return nedPath(); }

    double area() const;

    QGeoCoordinate center() const { return _center; }
    bool centerDrag() const { return _centerDrag; }
    bool showAltColor() const { return _showAltColor; }

    void setCenter(QGeoCoordinate newCenter);
    void setCenterDrag(bool centerDrag);
    void setShowAltColor(bool showAltColor);

    static constexpr const char* jsonPolygonKey = "polygon";

signals:
    void centerChanged(QGeoCoordinate center);
    void centerDragChanged(bool centerDrag);
    void showAltColorChanged(bool showAltColor);

protected:
    int _minVertexCount() const override { return 3; }
    bool _closedPath() const override { return true; }

    void _onPathChanged() override;
    void _onModelReset() override;
    bool _loadFromFile(const QString& file, QList<QList<QGeoCoordinate>>& coords, QString& errorString) override;

    const char* _jsonKey() const override { return jsonPolygonKey; }
    void _clearImpl() override;

    QString _emptyFileMessage() const override { return tr("No polygons found in file"); }

private slots:
    void _updateCenter();

private:
    void _scheduleDeferredCenterChanged(const QGeoCoordinate& center);
    QPolygonF _toPolygonF() const;

    QGeoCoordinate _center;
    bool _centerDrag = false;
    bool _ignoreCenterUpdates = false;
    bool _showAltColor = false;
    bool _deferredCenterChanged = false;
    QGeoCoordinate _pendingCenter;
};
