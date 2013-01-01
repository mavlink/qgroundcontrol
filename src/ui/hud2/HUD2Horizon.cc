#include <QtGui>
#include "HUD2Horizon.h"

HUD2Horizon::HUD2Horizon(HUD2data *huddata, QWidget *parent) :
    QWidget(parent),
    pitchlinepos(&this->gap, parent)
{
    this->huddata = huddata;
}

void HUD2Horizon::updateGeometry(const QSize *size){
    const int gapscale = 13;
    gap = size->width() / gapscale;
    pitchlinepos.updateGeometry(size);

    int x1 = size->width();
    pen.setWidth(6);
    left.setLine(-x1, 0, -gap/2, 0);
    right.setLine(gap/2, 0, x1, 0);
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
    while (i > -180){
        painter->translate(0, -pixstep);
        pitchlinepos.paint(painter);
        i -= degstep;
    }

    painter->restore();
}

/**
 * @brief HUD2Horizon::drawwings
 * @param painter
 */
void HUD2Horizon::drawwings(QPainter *painter, QColor color){
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawLine(left);
    painter->drawLine(right);
}

/**
 * @brief HUD2Horizon::paint
 * @param painter
 * @param color
 */
void HUD2Horizon::paint(QPainter *painter, QColor color){

    painter->save();

    qreal deginpix = 0.2;
    qreal degstep = 20;
    qreal pixstep = degstep / deginpix;
    qreal pitch_deg = rad2deg(huddata->pitch);

    QTransform transform;
    QPoint center = painter->viewport().center();
    transform.translate(center.x(), center.y());
    transform.translate(tan(huddata->roll) * (pitch_deg * pixstep), pitch_deg * pixstep);
    transform.rotate(-rad2deg(huddata->roll));
    painter->setTransform(transform);

    drawpitchlines(painter, degstep, pixstep);
    drawwings(painter, color);

    painter->restore();
}

void HUD2Horizon::setColor(QColor color){
    pen.setColor(color);
}

qreal HUD2Horizon::rad2deg(float rad){
    return rad * (180.0 / M_PI);
}
