/****************************************************************************
 *
 *   (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapGridMGRS.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

//-----------------------------------------------------------------------------
MapGridMGRS::MapGridMGRS()
    : QObject()
{
}

//-----------------------------------------------------------------------------
void
MapGridMGRS::geometryChanged(double zoomLevel, QGeoCoordinate topLeft, QGeoCoordinate bottomRight)
{
    if (!topLeft.isValid() || !bottomRight.isValid()) {
        emit updateValues(QVariant());
        return;
    }
    qCritical() << "Zoom: " << zoomLevel << "Viewport: " << topLeft << " ; " << bottomRight;

    QJsonObject p1;
    p1.insert(QStringLiteral("lat"), (topLeft.latitude() + bottomRight.latitude()) / 2);
    p1.insert(QStringLiteral("lng"), topLeft.longitude());
    QJsonObject p2;
    p2.insert(QStringLiteral("lat"), (topLeft.latitude() + bottomRight.latitude()) / 2);
    p2.insert(QStringLiteral("lng"), bottomRight.longitude());
    QJsonArray line1pts;
    line1pts.push_back(p1);
    line1pts.push_back(p2);

    QJsonObject line1a;
    line1a.insert(QStringLiteral("points"), line1pts);
    line1a.insert(QStringLiteral("color"), "#77ffffff");
    line1a.insert(QStringLiteral("width"), 1);
    QJsonObject line1b;
    line1b.insert(QStringLiteral("points"), line1pts);
    line1b.insert(QStringLiteral("color"), "#44000000");
    line1b.insert(QStringLiteral("width"), 3);

    QJsonObject p3;
    p3.insert(QStringLiteral("lat"), topLeft.latitude());
    p3.insert(QStringLiteral("lng"), (topLeft.longitude() + bottomRight.longitude()) / 2);
    QJsonObject p4;
    p4.insert(QStringLiteral("lat"), bottomRight.latitude());
    p4.insert(QStringLiteral("lng"), (topLeft.longitude() + bottomRight.longitude()) / 2);
    QJsonArray line2pts;
    line2pts.push_back(p3);
    line2pts.push_back(p4);

    QJsonObject line2a;
    line2a.insert(QStringLiteral("points"), line2pts);
    line2a.insert(QStringLiteral("color"), "#77ffffff");
    line2a.insert(QStringLiteral("width"), 1);
    QJsonObject line2b;
    line2b.insert(QStringLiteral("points"), line2pts);
    line2b.insert(QStringLiteral("color"), "#44000000");
    line2b.insert(QStringLiteral("width"), 3);

    QJsonArray lines;
    lines.push_back(line1a);
    lines.push_back(line1b);
    lines.push_back(line2a);
    lines.push_back(line2b);

    emit updateValues(QVariant(lines));
}
//-----------------------------------------------------------------------------
