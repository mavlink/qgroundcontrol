#include "MapMarkerGrouping.h"

#include <cmath>
#include <limits>
#include <utility>

#include <QtCore/QHash>

MapMarkerGrouping::MapMarkerGrouping(QObject* parent)
    : QObject(parent)
{}

QList<int> MapMarkerGrouping::representatives(const QList<QPointF>& points, double distance)
{
    QList<int> result;
    result.reserve(points.size());

    // Representatives bucketed by distance-sized cells: only the 3x3 neighborhood can be in range
    QHash<std::pair<qint64, qint64>, QList<int>> cells;
    const double distanceSquared = distance * distance;

    for (qsizetype i = 0; i < points.size(); i++) {
        const QPointF& point = points[i];
        const int index = static_cast<int>(i);
        if ((distance <= 0.0) || !std::isfinite(point.x()) || !std::isfinite(point.y())) {
            result.append(index);
            continue;
        }

        const qint64 cellX = static_cast<qint64>(std::floor(point.x() / distance));
        const qint64 cellY = static_cast<qint64>(std::floor(point.y() / distance));
        int closest = -1;
        double closestDistanceSquared = std::numeric_limits<double>::infinity();
        for (qint64 x = cellX - 1; x <= cellX + 1; x++) {
            for (qint64 y = cellY - 1; y <= cellY + 1; y++) {
                for (const int representative : cells.value({x, y})) {
                    const QPointF delta = point - points[representative];
                    const double candidate = QPointF::dotProduct(delta, delta);
                    if ((candidate <= distanceSquared) &&
                        ((candidate < closestDistanceSquared) ||
                         ((candidate == closestDistanceSquared) && (representative < closest)))) {
                        closest = representative;
                        closestDistanceSquared = candidate;
                    }
                }
            }
        }

        if (closest >= 0) {
            result.append(closest);
        } else {
            result.append(index);
            cells[{cellX, cellY}].append(index);
        }
    }

    return result;
}
