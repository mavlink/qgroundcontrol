#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtQmlIntegration/QtQmlIntegration>

/// Groups map markers drawn too close together on screen to click individually,
/// for the overlapping-marker picker of both map engines (MissionItemIndicatorGroup
/// and GeoMapMissionItems).
class MapMarkerGrouping : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit MapMarkerGrouping(QObject* parent = nullptr);

    /// Greedy in input order, so callers pass markers by priority: each point joins
    /// the group whose representative (first point) is nearest and within distance,
    /// ties going to the earlier group, else it starts its own group. Returns the
    /// representative's index for each point. A non-finite point is its own group,
    /// and a non-positive distance groups nothing.
    Q_INVOKABLE static QList<int> representatives(const QList<QPointF>& points, double distance);
};
