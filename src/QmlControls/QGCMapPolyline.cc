#include "QGCMapPolyline.h"

#include <QtCore/QLineF>

#include "GeoFormatRegistry.h"
#include "QGCGeo.h"

QGCMapPolyline::QGCMapPolyline(QObject* parent)
    : QGCMapPathBase(parent)
{
}

QGCMapPolyline::QGCMapPolyline(const QGCMapPolyline& other, QObject* parent)
    : QGCMapPathBase(parent)
{
    *this = other;
}

const QGCMapPolyline& QGCMapPolyline::operator=(const QGCMapPolyline& other)
{
    QGCMapPathBase::operator=(other);
    return *this;
}

bool QGCMapPolyline::_loadFromFile(const QString& file, QList<QList<QGeoCoordinate>>& coords, QString& errorString)
{
    return GeoFormatRegistry::loadPolylines(file, coords, errorString);
}

double QGCMapPolyline::length() const
{
    double length = 0;

    for (int i = 0; i < _path.count() - 1; i++) {
        const QGeoCoordinate from = _path[i].value<QGeoCoordinate>();
        const QGeoCoordinate to = _path[i + 1].value<QGeoCoordinate>();
        length += from.distanceTo(to);
    }

    return length;
}

QList<QGeoCoordinate> QGCMapPolyline::offsetPolyline(double distance)
{
    QList<QGeoCoordinate> rgNewPolyline;

    if (count() > 1) {
        const QList<QPointF> rgNedVertices = nedPath();

        QList<QLineF> rgOffsetEdges;
        for (int i = 0; i < rgNedVertices.count() - 1; i++) {
            QLineF offsetEdge;
            const QLineF originalEdge(rgNedVertices[i], rgNedVertices[i + 1]);

            QLineF workerLine = originalEdge;
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() - 90.0);
            offsetEdge.setP1(workerLine.p2());

            workerLine.setPoints(originalEdge.p2(), originalEdge.p1());
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() + 90.0);
            offsetEdge.setP2(workerLine.p2());

            rgOffsetEdges.append(offsetEdge);
        }

        const QGeoCoordinate tangentOrigin = vertexCoordinate(0);

        QGeoCoordinate coord;
        QGCGeo::convertNedToGeo(rgOffsetEdges[0].p1().y(), rgOffsetEdges[0].p1().x(), 0, tangentOrigin, coord);
        rgNewPolyline.append(coord);

        QPointF newVertex;
        for (int i = 1; i < rgOffsetEdges.count(); i++) {
            const auto intersect = rgOffsetEdges[i - 1].intersects(rgOffsetEdges[i], &newVertex);
            if (intersect == QLineF::NoIntersection) {
                newVertex = rgOffsetEdges[i].p2();
            }
            QGCGeo::convertNedToGeo(newVertex.y(), newVertex.x(), 0, tangentOrigin, coord);
            rgNewPolyline.append(coord);
        }

        const int lastIndex = rgOffsetEdges.count() - 1;
        QGCGeo::convertNedToGeo(rgOffsetEdges[lastIndex].p2().y(), rgOffsetEdges[lastIndex].p2().x(), 0, tangentOrigin,
                                coord);
        rgNewPolyline.append(coord);
    }

    return rgNewPolyline;
}
