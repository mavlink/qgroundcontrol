#include "ShapeVertices.h"

#include <algorithm>

void ShapeVertices::removeClosingVertex(QList<QGeoCoordinate>& vertices)
{
    constexpr double closureThresholdMeters = 0.01;
    if (vertices.count() > 3 && vertices.last().distanceTo(vertices.first()) < closureThresholdMeters) {
        vertices.removeLast();
    }
}

void ShapeVertices::makeClockwise(QList<QGeoCoordinate>& vertices)
{
    double sum = 0;
    for (qsizetype i = 0; i < vertices.count(); i++) {
        const QGeoCoordinate& coord1 = vertices[i];
        const QGeoCoordinate& coord2 = vertices[(i + 1) % vertices.count()];
        sum += (coord2.longitude() - coord1.longitude()) * (coord2.latitude() + coord1.latitude());
    }
    if (sum < 0.0) {
        std::reverse(vertices.begin(), vertices.end());
    }
}

void ShapeVertices::filterNearby(QList<QGeoCoordinate>& vertices, double filterMeters, int minVertices)
{
    if (filterMeters <= 0) {
        return;
    }

    qsizetype i = 0;
    while (vertices.count() > minVertices && i < vertices.count() - 1) {
        if (vertices[i].distanceTo(vertices[i + 1]) < filterMeters) {
            vertices.removeAt(i + 1);
        } else {
            i++;
        }
    }
}
