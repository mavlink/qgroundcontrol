#include <QtGui>

#include "HUD2Horizon.h"
#include "HUD2Math.h"

HUD2Horizon::HUD2Horizon(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    pitchline(&this->gap, this),
    crosshair(&this->gap, this),
    huddata(huddata)
{
    this->gap = 6;
    this->pitchcount = 5;
    this->degstep = 10;
}

void HUD2Horizon::updateGeometry(const QSize &size){
    int _gap = percent2pix_w(size, this->gap);

    // wings
    int x1 = size.width() / 2;
    int tmp = percent2pix_h(size, 1);
    hud2_clamp(tmp, 2, 10);
    pen.setWidth(tmp);
    hirizonleft.setLine(-x1, 0, -_gap, 0);
    horizonright.setLine(_gap, 0, x1, 0);
    
    // pitchlines
    pixstep = size.height() / pitchcount;
    pitchline.updateGeometry(size);

    // crosshair
    crosshair.updateGeometry(size);
}

/**
 * @brief drawpitchlines
 * @param painter
 * @param degstep
 * @param pixstep
 */
void HUD2Horizon::drawpitchlines(QPainter *painter, qreal degstep, qreal pixstep){

    painter->save();
    int i = 0;
    while (i > -360){
        i -= degstep;
        painter->translate(0, -pixstep);
        pitchline.paint(painter, -i);
    }
    painter->restore();

    painter->save();
    i = 0;
    while (i < 360){
        i += degstep;
        painter->translate(0, pixstep);
        pitchline.paint(painter, -i);
    }
    painter->restore();
}

/**
 * @brief HUD2Horizon::drawwings
 * @param painter
 */
void HUD2Horizon::drawhorizon(QPainter *painter){
    pen.setColor(Qt::green);
    painter->setPen(pen);
    painter->drawLine(hirizonleft);
    painter->drawLine(horizonright);
}

void HUD2Horizon::paint(QPainter *painter){

    //
    crosshair.paint(painter);

    painter->save();

    // now perform complex transfomation of painter
    qreal alpha = rad2deg(-huddata.pitch);

    QTransform transform;
    QPoint center;

    center = painter->window().center();
    transform.translate(center.x(), center.y());
    qreal delta_y = alpha * (pixstep / degstep);
    qreal delta_x = tan(-huddata.roll) * delta_y;

    transform.translate(delta_x, delta_y);
    transform.rotate(rad2deg(huddata.roll));

    painter->setTransform(transform);
    drawpitchlines(painter, degstep, pixstep);
    drawhorizon(painter);

    painter->restore();
}

void HUD2Horizon::setColor(QColor color){
    pen.setColor(color);
}
