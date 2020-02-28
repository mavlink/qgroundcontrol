/****************************************************************************
 *
 *   (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QColor>
#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoRectangle>
#include <QMap>
#include <QQueue>
#include <QVariant>
#include <memory>

//=============================================================================
class MGRSLabel
{
public:
    MGRSLabel(const QString& label, const QGeoCoordinate& pos, const QString& foregroundColor, const QString& backgroundColor)
    {
        _label = label;
        _pos = pos;
        _foregroundColor = foregroundColor;
        _backgroundColor = backgroundColor;
    }

    QString _label;
    QGeoCoordinate _pos;
    QString _foregroundColor;
    QString _backgroundColor;
};

//=============================================================================
// Represents MGRS zones defined by first 5 characters of MGRS coordinate
// For example: 32UQU is composed of level 1 zone 32U and level 2 zone QU

class MGRSZone
{
public:
    MGRSZone(QString _label);

    bool visited = false;
    bool valid = true;
    bool leftOverlap = false;
    bool rightOverlap = false;
    QString label;
    QGeoCoordinate bottomLeft;
    QGeoCoordinate topLeft;
    QGeoCoordinate bottomRight;
    QGeoCoordinate topRight;
    QGeoCoordinate labelPos;
    QGeoCoordinate searchPos;
    QGeoCoordinate rightSearchPos;
    QGeoCoordinate leftSearchPos;
    QGeoCoordinate topSearchPos;
    QGeoCoordinate bottomSearchPos;

    QGeoRectangle rect();

private:
    // Some level 2 zones overlap. Instead of drawing overlapping rectangles we crop the edges to
    // level1 zone lines. This method finds the edge of the level1 zone specified with l1l and returns
    // cropped edge. start, dir and format specify the beginning and direction of search
    void _fixEdge(QString l1l, int start, int dir, QString format, QGeoCoordinate& edgeToFix);
};

//=============================================================================
class MapGridMGRS : public QObject
{
    Q_OBJECT
public:
    explicit MapGridMGRS() {};

public slots:
    void geometryChanged(double zoomLevel, const QGeoCoordinate& topLeft, const QGeoCoordinate& bottomRight);

signals:
    void updateValues(const QVariant& values);

private:
    const double maxLevel1LabelsZoomLevel = 4;
    const double maxZoneZoomLevel = 7;
    const double leve3ZoomLevel = 10;
    const int maxNumberOfLinesOnScreen = 10;
    const int maxZoneMapCacheSize = 2000;

    const QString level1LineForegroundColor = "#AAFF9999";
    const int level1LineForgroundWidth = 3;
    const QString level1LineBackgroundColor = "#55660000";
    const int level1LineBackgroundWidth = 5;
    const QString level1LabelForegroundColor = "#CCFF9999";
    const QString level1LabelBackgroundColor = "#88660000";

    const QString level2LineForegroundColor = "#AAb3FFb3";
    const int level2LineForgroundWidth = 2;
    const QString level2LineBackgroundColor = "#55006600";
    const int level2LineBackgroundWidth = 4;
    const QString level2LabelForegroundColor = "#CCb3FFb3";
    const QString level2LabelBackgroundColor = "#88006600";

    const QString level3LineForegroundColor = "#77FFFFFF";
    const int level3LineForgroundWidth = 1;
    const QString level3LineBackgroundColor = "#44000000";
    const int level3LineBackgroundWidth = 3;
    const QString level3LabelForegroundColor = "#CCFFFFFF";
    const QString level3LabelBackgroundColor = "#88000000";

    QGeoRectangle _currentViewportRect;
    double _zoomLevel = 0;
    double _minDistanceBetweenLines = 0;
    QMap<QString, std::shared_ptr<MGRSZone>> _zoneMap;
    QQueue<QString> _zoneMapQueue;
    QList<QGeoPath> _level1lines;
    QList<MGRSLabel> _level1labels;
    QList<QGeoPath> _level1Paths;
    QList<QGeoPath> _level2Paths;
    QList<QGeoPath> _level3Paths;
    QList<MGRSLabel> _mgrsLabels;

    bool _lineIntersectsLine(const QGeoCoordinate& l1p1, const QGeoCoordinate& l1p2, const QGeoCoordinate& l2p1, const QGeoCoordinate& l2p2);

    bool _lineIntersectsRect(const QGeoCoordinate& p1, const QGeoCoordinate& p2, const QGeoRectangle& r);

    bool _rectOverlapsRect(const QGeoRectangle& r1, const QGeoRectangle& r2);

    void _clear();

    void _addLevel1Lines();

    void _addLevel1Labels();

    void _findZoneBoundaries(const QGeoCoordinate& start);

    void _createLevel3Paths(std::shared_ptr<MGRSZone> &tile);

    void _addLines(QJsonArray& lines, const QList<QGeoPath>& paths, const QString& color1, int width1, const QString& color2, int width2);

    void _addLabels(QJsonArray& labels);
};

//=============================================================================
