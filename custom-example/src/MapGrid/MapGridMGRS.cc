/****************************************************************************
 *
 *   (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapGridMGRS.h"
#include "QGCGeo.h"
#include <QJsonObject>
#include <QJsonArray>
#include <math.h>

#include <QLoggingCategory>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>

static QString level1Label(QString mgrs) { return mgrs.left(3); }

static QString level2Label(QString mgrs) { return mgrs.mid(3, 2); }

static QString zoneLabel(QString mgrs) { return mgrs.left(5); }

//=============================================================================
MGRSZone::MGRSZone(QString _label)
{
    label = _label;

    if (!convertMGRSToGeo(label + "0000000000", bottomLeft)) {
        valid = false;
    }

    if (!convertMGRSToGeo(label + "2000050000", searchPos)) {
        valid = false;
    }

    if (!convertMGRSToGeo(label + "0000099999", topLeft)) {
        valid = false;
    }

    if (!convertMGRSToGeo(label + "9999900000", bottomRight)) {
        valid = false;
    }

    if (!convertMGRSToGeo(label + "9999999999", topRight)) {
        valid = false;
    }

    QString l2l = level2Label(label);

    // top Right overlap
    if (level2Label(convertGeoToMGRS(topRight)) != l2l) {
        _fixEdge(l2l, 100000, -1, "%1 99999", topRight);
    }

    // top Left overlap
    if (level2Label(convertGeoToMGRS(topLeft)) != l2l) {
        leftOverlap = true;
        _fixEdge(l2l, 0, 1, "%1 99999", topLeft);
    }

    // Bottom Right overlap
    if (level2Label(convertGeoToMGRS(bottomRight)) != l2l) {
        rightOverlap = true;
        _fixEdge(l2l, 100000, -1, "%1 00000", bottomRight);
    }

    // Bottom Left overlap
    if (level2Label(convertGeoToMGRS(bottomLeft)) != l2l) {
        leftOverlap = true;
        _fixEdge(l2l, 0, 1, "%1 00000", bottomLeft);
    }

    if (!convertMGRSToGeo(label + "5000050000", labelPos)) {
        rightOverlap = true;
        valid = false;
    }
    labelPos.setLongitude((topLeft.longitude() + topRight.longitude() + bottomLeft.longitude() + bottomRight.longitude()) / 4);

    QString searchLabel;

    if (!convertMGRSToGeo(label + "9999950000", rightSearchPos)) {
        valid = false;
    }
    rightSearchPos = rightSearchPos.atDistanceAndAzimuth(100, 90);

    if (!convertMGRSToGeo(label + "0000050000", leftSearchPos)) {
        valid = false;
    }
    leftSearchPos = leftSearchPos.atDistanceAndAzimuth(100, 270);

    if (!convertMGRSToGeo(label + "5000000000", bottomSearchPos)) {
        valid = false;
    }
    bottomSearchPos = bottomSearchPos.atDistanceAndAzimuth(100, 180);

    if (!convertMGRSToGeo(label + "5000099999", topSearchPos)) {
        valid = false;
    }
    topSearchPos = topSearchPos.atDistanceAndAzimuth(100, 0);
}

//-----------------------------------------------------------------------------
QGeoRectangle
MGRSZone::rect()
{
    QGeoRectangle r(topLeft, bottomRight);
    r.setTopRight(topRight);
    r.setBottomLeft(bottomLeft);
    return r;
}

//-----------------------------------------------------------------------------
void
MGRSZone::_fixEdge(QString l2l, int start, int dir, QString format, QGeoCoordinate& edgeToFix)
{
    QString mgrsC;
    QGeoCoordinate c;
    int coord = start;

    for (double step = 10000; step >= 1; step /= 10) {
        do {
            coord += step * dir;
            mgrsC = label + " " + QString(format).arg(coord, 5, 10, QChar('0'));
            if (!convertMGRSToGeo(mgrsC, c)) {
                break;
            }
        } while (level2Label(convertGeoToMGRS(c)) != l2l);
        coord -= step * dir;
    }
    convertMGRSToGeo(mgrsC, edgeToFix);
}

//=============================================================================
bool
MapGridMGRS::_lineIntersectsLine(const QGeoCoordinate& l1p1, const QGeoCoordinate& l1p2, const QGeoCoordinate& l2p1, const QGeoCoordinate& l2p2)
{
    double q = (l1p1.latitude() - l2p1.latitude()) * (l2p2.longitude() - l2p1.longitude()) - (l1p1.longitude() - l2p1.longitude()) * (l2p2.latitude() - l2p1.latitude());
    double d = (l1p2.longitude() - l1p1.longitude()) * (l2p2.latitude() - l2p1.latitude()) - (l1p2.latitude() - l1p1.latitude()) * (l2p2.longitude() - l2p1.longitude());

    if (d == 0) {
        return false;
    }
    double r = q / d;
    q = (l1p1.latitude() - l2p1.latitude()) * (l1p2.longitude() - l1p1.longitude()) - (l1p1.longitude() - l2p1.longitude()) * (l1p2.latitude() - l1p1.latitude());
    double s = q / d;

    if (r < 0 || r > 1 || s < 0 || s > 1) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool
MapGridMGRS::_lineIntersectsRect(const QGeoCoordinate& p1, const QGeoCoordinate& p2, const QGeoRectangle& r)
{
    return
        _lineIntersectsLine(p1, p2, r.topRight(), r.topLeft()) ||
        _lineIntersectsLine(p1, p2, r.topLeft(), r.bottomLeft()) ||
        _lineIntersectsLine(p1, p2, r.bottomLeft(), r.bottomRight()) ||
        _lineIntersectsLine(p1, p2, r.bottomRight(), r.topRight()) ||
        (r.contains(p1) && r.contains(p2));
}


//-----------------------------------------------------------------------------
bool
MapGridMGRS::_rectOverlapsRect(const QGeoRectangle& r1, const QGeoRectangle& r2)
{
    double aLeft = std::min(r1.topLeft().longitude(), r1.bottomLeft().longitude());
    double aRight = std::max(r1.topRight().longitude(), r1.bottomRight().longitude());
    double aTop = std::max(r1.topLeft().latitude(), r1.topRight().latitude());
    double aBottom = std::min(r1.bottomLeft().latitude(), r1.bottomRight().latitude());

    double bLeft = std::min(r2.topLeft().longitude(), r2.bottomLeft().longitude());
    double bRight = std::max(r2.topRight().longitude(), r2.bottomRight().longitude());
    double bTop = std::max(r2.topLeft().latitude(), r2.topRight().latitude());
    double bBottom = std::min(r2.bottomLeft().latitude(), r2.bottomRight().latitude());


    return aLeft < bRight && aRight > bLeft && aTop > bBottom && aBottom < bTop;
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::geometryChanged(double zoomLevel, const QGeoCoordinate& topLeft, const QGeoCoordinate& bottomRight)
{
    if (zoomLevel < 3) {
        emit updateValues(QVariant());
        return;
    }
    bool zoomingIn = zoomLevel > _zoomLevel;
    if (topLeft.isValid() && bottomRight.isValid() &&
        (!_currentViewportRect.contains(topLeft) || !_currentViewportRect.contains(bottomRight) ||
         (zoomingIn && zoomLevel > maxZoneZoomLevel) ||
         (_zoomLevel <= maxLevel1LabelsZoomLevel && zoomLevel > maxLevel1LabelsZoomLevel))) {

        _zoomLevel = zoomLevel;

        double latDiff = (topLeft.latitude() - bottomRight.latitude()) / 3;
        double topLat = topLeft.latitude() + latDiff;
        if (topLat > 84) {
            topLat = 84;
        }
        double bottomLat = bottomRight.latitude() - latDiff;
        if (bottomLat < -80) {
            bottomLat = -80;
        }
        double lngDiff = (bottomRight.longitude() - topLeft.longitude()) / 3;
        double leftLng;
        double rightLng;
        if (lngDiff < 0) {
            lngDiff *= -1;
            leftLng = bottomRight.longitude() - lngDiff;
            rightLng = topLeft.longitude() + lngDiff;
        } else {
            leftLng = topLeft.longitude() - lngDiff;
            rightLng = bottomRight.longitude() + lngDiff;
        }
        if (leftLng < -180) {
            leftLng = -180;
        }
        if (rightLng > 180) {
            rightLng = 180;
        }
        _currentViewportRect.setTopLeft(QGeoCoordinate(topLat, leftLng));
        _currentViewportRect.setTopRight(QGeoCoordinate(topLat, rightLng));
        _currentViewportRect.setBottomLeft(QGeoCoordinate(bottomLat, leftLng));
        _currentViewportRect.setBottomRight(QGeoCoordinate(bottomLat, rightLng));

        double centerLat = (topLeft.latitude() + bottomRight.latitude()) / 2;
        QGeoCoordinate centerLeft = QGeoCoordinate(centerLat, topLeft.longitude());
        QGeoCoordinate centerRight = QGeoCoordinate(centerLat, bottomRight.longitude());
        _minDistanceBetweenLines = centerLeft.distanceTo(centerRight) / maxNumberOfLinesOnScreen;
    } else {
        emit updateValues(QVariant(false));
        return;
    }

    _clear();
    _addLevel1Lines();
    _addLevel1Labels();

    QJsonArray lines;
    _addLines(lines, _level1Paths, level1LineBackgroundColor, level1LineBackgroundWidth, level1LineForegroundColor, level1LineForgroundWidth);

    if (_zoomLevel > maxZoneZoomLevel) {
        _findZoneBoundaries(QGeoCoordinate((topLeft.latitude() + bottomRight.latitude()) / 2, (topLeft.longitude() + bottomRight.longitude()) / 2));
        _addLines(lines, _level2Paths, level2LineBackgroundColor, level2LineBackgroundWidth, level2LineForegroundColor, level2LineForgroundWidth);
        _addLines(lines, _level3Paths, level3LineBackgroundColor, level3LineBackgroundWidth, level3LineForegroundColor, level3LineForgroundWidth);
        while (_zoneMapQueue.count() > maxZoneMapCacheSize) {
            _zoneMap.remove(_zoneMapQueue.dequeue());
        }
    }

    QJsonArray labels;
    _addLabels(labels);

    QJsonObject values;
    values.insert(QStringLiteral("lines"), lines);
    values.insert(QStringLiteral("labels"), labels);

    emit updateValues(QVariant(values));
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::_addLevel1Lines()
{
    if (_level1lines.empty()) {
        for (int lat = -80; lat <= 84; lat += (lat < 70) ? 8 : 12) {
            int step = 60;
            for (int lng = -180; lng < 180; lng += step) {
                QGeoPath path;
                path.addCoordinate(QGeoCoordinate(lat, lng));
                path.addCoordinate(QGeoCoordinate(lat, lng + step));
                _level1lines.push_back(path);
            }
        }
        for (int lng = -180; lng <= 180; lng += 6) {
            QGeoPath path;
            if (lng == 6) {
                // Norway anomaly
                path.addCoordinate(QGeoCoordinate(-80, lng));
                path.addCoordinate(QGeoCoordinate(56, lng));
                QGeoPath path2;
                path2.addCoordinate(QGeoCoordinate(56, lng - 3));
                path2.addCoordinate(QGeoCoordinate(64, lng - 3));
                _level1lines.push_back(path2);
                QGeoPath path3;
                path3.addCoordinate(QGeoCoordinate(64, lng));
                path3.addCoordinate(QGeoCoordinate(72, lng));
                _level1lines.push_back(path3);
            } else if (lng >= 12 && lng <= 36 ) {
                // Svalbard anomaly
                path.addCoordinate(QGeoCoordinate(-80, lng));
                path.addCoordinate(QGeoCoordinate(72, lng));
            } else {
                path.addCoordinate(QGeoCoordinate(-80, lng));
                path.addCoordinate(QGeoCoordinate(84, lng));
            }
            _level1lines.push_back(path);
        }
        for (int lng = 9; lng <= 33; lng += 12) {
            // Svalbard anomaly
            QGeoPath path;
            path.addCoordinate(QGeoCoordinate(72, lng));
            path.addCoordinate(QGeoCoordinate(84, lng));
            _level1lines.push_back(path);
        }
    }

    if (_zoomLevel > 3) {
        for (int i = 0; i < _level1lines.count(); i++) {
            if (_zoomLevel < 6 || _lineIntersectsRect(_level1lines[i].coordinateAt(0), _level1lines[i].coordinateAt(1), _currentViewportRect)) {
                _level1Paths.push_back(_level1lines[i]);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::_addLevel1Labels()
{
    if (_level1labels.empty()) {
        for (int lng = -180; lng < 180; lng += 6) {
            for (int lat = -80; lat <= 72; lat += (lat < 70) ? 8 : 12) {
                if ((lat == 72 && (lng == 6 || lng == 18 || lng == 30))) {
                    continue;
                }
                QString text = level1Label(convertGeoToMGRS(QGeoCoordinate(lat, lng)));
                double llat = lat + 4;
                double llng = lng + 3;
                if (text == "31V") {
                    llng = lng + 1.75;
                } else if (text == "31X") {
                    llng = lng + 4.5;
                } else if (text == "32V") {
                    llng = lng + 2;
                } else if (text == "37X") {
                    llng = lng + 1.5;
                }

                QGeoCoordinate pos(llat, llng);
                _level1labels.push_back(MGRSLabel(text, pos, level1LabelForegroundColor, level1LabelBackgroundColor));
            }
        }
    }

    if (_zoomLevel > maxLevel1LabelsZoomLevel && _zoomLevel <= maxZoneZoomLevel) {
        for (int i = 0; i < _level1labels.count(); i++) {
            if (_zoomLevel < 6 || _currentViewportRect.contains(_level1labels[i]._pos)) {
                _mgrsLabels.push_back(_level1labels[i]);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::_clear()
{
    _level1Paths.clear();
    _level2Paths.clear();
    _level3Paths.clear();
    _mgrsLabels.clear();

    for (auto i = _zoneMap.begin(); i != _zoneMap.end(); ++i)
        i.value()->visited = false;
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::_findZoneBoundaries(const QGeoCoordinate& pos)
{
    QString mgrsPos = convertGeoToMGRS(pos);
    QString label = zoneLabel(mgrsPos);

    std::shared_ptr<MGRSZone> tile = _zoneMap.value(label);
    if (!tile) {
        tile = std::shared_ptr<MGRSZone>(new MGRSZone(label));
        _zoneMap.insert(label, tile);
        _zoneMapQueue.enqueue(label);
    }

    if (tile->valid && !tile->visited && pos.latitude() < 84 && pos.latitude() > -80 &&
        (_currentViewportRect.contains(pos) || _rectOverlapsRect(tile->rect(), _currentViewportRect))) {
        tile->visited = true;

        _findZoneBoundaries(tile->topSearchPos);
        _findZoneBoundaries(tile->rightSearchPos);
        _findZoneBoundaries(tile->bottomSearchPos);
        _findZoneBoundaries(tile->leftSearchPos);

        QGeoPath path;
        if (!tile->leftOverlap) {
            path.addCoordinate(tile->topLeft);
        }
        path.addCoordinate(tile->bottomLeft);
        path.addCoordinate(tile->bottomRight);

        _level2Paths.push_back(path);

        if (_zoomLevel > maxZoneZoomLevel && _currentViewportRect.contains(tile->labelPos)) {
            _mgrsLabels.push_back(MGRSLabel(level2Label(tile->label), tile->labelPos, level2LabelForegroundColor, level2LabelBackgroundColor));
        }

        _createLevel3Paths(tile);
    }
}
//-----------------------------------------------------------------------------
void
MapGridMGRS::_createLevel3Paths(std::shared_ptr<MGRSZone> &tile)
{
    QGeoCoordinate c1;
    QGeoCoordinate c2;
    int distanceBetweenLines = 0;
    const int factors[] = { 500, 1000, 5000, 10000, 50000 };

    QGeoCoordinate bl;
    if (!convertMGRSToGeo(tile->label + "0000000000", bl)) {
        return;
    }

    QGeoRectangle tileRect(tile->topLeft, tile->bottomRight);
    tileRect.setTopRight(tile->topRight);
    tileRect.setBottomLeft(tile->bottomLeft);

    bool overlapped = tile->leftOverlap || tile->rightOverlap;

    for (int i = 0; i <= 4; i ++) {
        if (!convertMGRSToGeo(tile->label + QString("%1").arg(factors[i], 5, 10, QChar('0')) + "00000", c1)) {
            return;
        }
        if (bl.distanceTo(c1) > _minDistanceBetweenLines) {
            distanceBetweenLines = factors[i];
            break;
        }
    }
    if (distanceBetweenLines <= 0) {
        return;
    }

    int cnt1 = 0;
    for (int i = distanceBetweenLines; i < 100000; i += distanceBetweenLines) {
        cnt1++;
        // Horizontal lines
        QString coord = QString("%1").arg(i, 5, 10, QChar('0'));
        if (!convertMGRSToGeo(tile->label + "00000" + coord, c1)) {
            return;
        }
        int added = 0;
        QGeoPath path;
        if (!overlapped || tileRect.contains(c1)) {
            path.addCoordinate(c1);
            added++;
        }
        for (int j = distanceBetweenLines; j <= 100000; j += distanceBetweenLines) {
            if (j == 100000)
                j--;
            QString coordE = QString("%1").arg(j, 5, 10, QChar('0'));
            if (!convertMGRSToGeo(tile->label + coordE + coord, c2)) {
                break;
            }
            if (_lineIntersectsRect(c1, c2, _currentViewportRect) && (!overlapped || tileRect.contains(c2))) {
                path.addCoordinate(c2);
                added++;
            } else if (added > 2) {
                break;
            }
            c1 = c2;
        }
        if (added > 1) {
            _level3Paths.push_back(path);
        }

        // Vertical lines
        if (!convertMGRSToGeo(tile->label + coord + "00000", c1)) {
            return;
        }

        added = 0;
        path.clearPath();
        if (!overlapped || tileRect.contains(c1)) {
            path.addCoordinate(c1);
            added++;
        }
        int cnt2 = 0;
        for (int j = distanceBetweenLines; j <= 100000; j += distanceBetweenLines) {
            cnt2++;
            if (j == 100000)
                j--;
            QString coordN = QString("%1").arg(j, 5, 10, QChar('0'));
            if (!convertMGRSToGeo(tile->label + coord + coordN, c2)) {
                break;
            }
            if (_lineIntersectsRect(c1, c2, _currentViewportRect) && (!overlapped || tileRect.contains(c2))) {
                path.addCoordinate(c2);
                added++;
                if (added > 1 && cnt1 % 2 == 1 && cnt2 % 2 == 1 &&_zoomLevel > maxZoneZoomLevel &&
                    !(coord == "50000" && coordN == "50000") && _currentViewportRect.contains(c2)) {
                    QString text = coord.left(2) + " " + coordN.left(2);
                    _mgrsLabels.push_back(MGRSLabel(text, c2, level3LabelForegroundColor, level3LabelBackgroundColor));
                }
            } else if (added > 2) {
                break;
            }
            c1 = c2;
        }
        if (added > 1) {
            _level3Paths.push_back(path);
        }
    }
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::_addLines(QJsonArray& lines, const QList<QGeoPath>& paths, const QString& color1, int width1, const QString& color2, int width2)
{
    for (int i = 0; i < paths.length(); i++) {
        QJsonArray pathPts;
        for (int j = 0; j < paths[i].size(); j++) {
            QGeoCoordinate c = paths[i].coordinateAt(j);
            QJsonObject p;
            p.insert(QStringLiteral("lat"), c.latitude());
            p.insert(QStringLiteral("lng"), c.longitude());
            pathPts.push_back(p);
        }
        if (pathPts.count() == 0) {
            continue;
        }
        QJsonObject line;
        line.insert(QStringLiteral("points"), pathPts);
        line.insert(QStringLiteral("color1"), color1);
        line.insert(QStringLiteral("width1"), width1);
        line.insert(QStringLiteral("color2"), color2);
        line.insert(QStringLiteral("width2"), width2);
        lines.push_back(line);
    }
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::_addLabels(QJsonArray& labels)
{
    for (int i = 0; i < _mgrsLabels.length(); i++) {
        QJsonObject label;
        label.insert(QStringLiteral("text"), _mgrsLabels[i]._label);
        label.insert(QStringLiteral("lat"), _mgrsLabels[i]._pos.latitude());
        label.insert(QStringLiteral("lng"), _mgrsLabels[i]._pos.longitude());
        label.insert(QStringLiteral("backgroundColor"), _mgrsLabels[i]._backgroundColor);
        label.insert(QStringLiteral("foregroundColor"), _mgrsLabels[i]._foregroundColor);
        labels.push_back(label);
    }
}

//-----------------------------------------------------------------------------
