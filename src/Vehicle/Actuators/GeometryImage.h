/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCPalette.h"
#include "Common.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQuick/QQuickImageProvider>
#include <QtGui/QPainter>


namespace GeometryImage {

/**
 * Renders an image of an airframe geometry (currently only multirotor)
 */
class VehicleGeometryImageProvider : public QQuickImageProvider
{
public:

    struct ImagePosition {
        ActuatorGeometry::Type type;
        int index;
        QPointF position;
        float radius;
    };

    void drawAxisIndicator(QPainter& p, const QPointF& origin, float fontSize, const QColor& color);

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

    static VehicleGeometryImageProvider* instance();

    int getHighlightedMotorIndexAtPos(const QPointF& position);

    QList<ActuatorGeometry>& actuators() { return _actuators; }

    int numMotors() const;

private:
    VehicleGeometryImageProvider();
    ~VehicleGeometryImageProvider() = default;

    QList<ActuatorGeometry> _actuators{};

    QList<ImagePosition> _actuatorImagePositions{}; ///< highlighted actuators image positions
    QGCPalette           _palette;
};

} // namespace GeometryImage

