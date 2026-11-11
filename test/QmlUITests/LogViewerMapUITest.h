#pragma once

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

#include "QmlUITestBase.h"

class QQuickItem;

/// UI tests for the map of the Analyze view Log Viewer page, on both map engines.
class LogViewerMapUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _showsLogPath_data();
    void _showsLogPath();

private:
    static bool _writePositionLog(const QString& filePath, const QList<QGeoCoordinate>& path);
    static QPointF _mapPoint(QQuickItem* map, const QGeoCoordinate& coordinate);
    static bool _pathFitsMap(QQuickItem* map, const QList<QGeoCoordinate>& path);
};
