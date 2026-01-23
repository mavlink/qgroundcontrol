#pragma once

#include "QGCMapPathBase.h"

class QGCMapPolyline : public QGCMapPathBase
{
    Q_OBJECT

public:
    explicit QGCMapPolyline(QObject* parent = nullptr);
    QGCMapPolyline(const QGCMapPolyline& other, QObject* parent = nullptr);

    ~QGCMapPolyline() override = default;

    const QGCMapPolyline& operator=(const QGCMapPolyline& other);

    QList<QGeoCoordinate> offsetPolyline(double distance);

    QList<QPointF> nedPolyline() const { return nedPath(); }

    double length() const;

    static constexpr const char* jsonPolylineKey = "polyline";

protected:
    int _minVertexCount() const override { return 2; }

    bool _loadFromFile(const QString& file, QList<QList<QGeoCoordinate>>& coords, QString& errorString) override;

    const char* _jsonKey() const override { return jsonPolylineKey; }

    QString _emptyFileMessage() const override { return tr("No polylines found in file"); }
};
