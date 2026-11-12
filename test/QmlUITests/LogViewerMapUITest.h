#pragma once

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

#include "QmlUITestBase.h"

class QQuickItem;

/// UI tests for the map of the Analyze view Log Viewer page, on both map engines.
class LogViewerMapUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    /// True once \a map, of either engine, shows all of \a coordinates, and large: a place far away or at world
    /// scale does not count.
    static bool mapShowsCoordinates(QQuickItem* map, const QList<QGeoCoordinate>& coordinates);

private slots:
    void _showsLogPath_data();
    void _showsLogPath();

private:
    static bool _writePositionLog(const QString& filePath, const QList<QGeoCoordinate>& path);
    static QPointF _mapPoint(QQuickItem* map, const QGeoCoordinate& coordinate);
};
