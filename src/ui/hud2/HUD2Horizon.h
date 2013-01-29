#ifndef HUDHORIZON_H
#define HUDHORIZON_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"
#include "HUD2HorizonPitch.h"
#include "HUD2HorizonCrosshair.h"
#include "HUD2HorizonRoll.h"
#include "HUD2HorizonYaw.h"

class HUD2Horizon : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Horizon(HUD2Data &huddata, QWidget *parent);
    void paint_static(QPainter *painter);
    void paint_dynamic(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

private:
    void drawpitchlines(QPainter *painter, qreal degstep, qreal pixstep);
    void drawhorizon(QPainter *painter);

    HUD2HorizonPitch pitchline;
    int degstep;    // vertical screen capacity in degrees
    qreal pixstep;  // pixels between two lines
    int pitchcount; // how many pitch lines can be fitted on screen (approximately)

    HUD2HorizonCrosshair crosshair;
    HUD2HorizonRoll roll;
    HUD2HorizonYaw yaw;

    qreal gap; /* space between right and left parts */
    HUD2Data &huddata;
    QPen pen;
    QLine hirizonleft;
    QLine horizonright;

    bool coloredBackground;
    QColor sky;
    QColor ground;
};

#endif // HUDHORIZON_H
