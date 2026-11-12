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
    void _chartCursorMovesPositionDot_data();
    void _chartCursorMovesPositionDot();

private:
    void _addEngineRows();
    /// The path of the log that the tests load: a place of its own, away from the map view the other views share
    static QList<QGeoCoordinate> _logPath();
    static bool _writePositionLog(const QString& filePath, const QList<QGeoCoordinate>& path);
    /// Starts the UI on the fetched engine and opens the Log Viewer page. Check QTest::currentTestFailed() on return.
    void _openLogViewer(bool geoMapEngine);
    /// Opens \a logFile in the page. Returns the map of the page's Map tab once the log's path is read: the tab is
    /// not shown yet. Null, with the test failed, when the page has not the map of the fetched engine.
    QQuickItem* _openLog(const QString& logFile, bool geoMapEngine);
    static QString _mapName(bool geoMapEngine);
};
