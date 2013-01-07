#include <QtGui>

#include "HUD2Horizon.h"
#include "HUD2Math.h"

HUD2Horizon::HUD2Horizon(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    pitchline(&this->gapscale, this),
    crosshair(&this->gapscale, this),
    rollindicator(huddata, this),
    huddata(huddata)
{
    this->gapscale = 13;
    this->pitchcount = 5;
    this->degstep = 10;
}

void HUD2Horizon::updateGeometry(const QSize *size){
    int gap = size->width() / gapscale;

    // wings
    int x1 = size->width();
    pen.setWidth(6);
    leftwing.setLine(-x1, 0, -gap/2, 0);
    rightwing.setLine(gap/2, 0, x1, 0);

    // pitchlines
    pixstep = size->height() / pitchcount;
    pitchline.updateGeometry(size);

    // crosshair
    crosshair.updateGeometry(size);

    // roll indicator
    rollindicator.updateGeometry(size);
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
void HUD2Horizon::drawwings(QPainter *painter, QColor color){
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawLine(leftwing);
    painter->drawLine(rightwing);
}

/**
 * @brief HUD2Horizon::paint
 * @param painter
 * @param color
 */
void HUD2Horizon::paint(QPainter *painter, QColor color){

    // roll indicator
    rollindicator.paint(painter, color);

    //
    crosshair.paint(painter, color);
    painter->save();

    // now perform complex transfomation of painter
    qreal alpha = rad2deg(huddata->pitch);

    QTransform transform;
    QPoint center;

    center = painter->window().center();
    transform.translate(center.x(), center.y());
    qreal delta_y = alpha * (pixstep / degstep);
    qreal delta_x = tan(huddata->roll) * delta_y;

    transform.translate(delta_x, delta_y);
    transform.rotate(-rad2deg(huddata->roll));

    painter->setTransform(transform);
    drawpitchlines(painter, degstep, pixstep);
    drawwings(painter, color);

    painter->restore();
}

void HUD2Horizon::setColor(QColor color){
    pen.setColor(color);
}
