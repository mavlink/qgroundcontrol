#include <QtGui>

#include "HUD2IndicatorHorizon.h"
#include "HUD2Math.h"

HUD2IndicatorHorizon::HUD2IndicatorHorizon(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    pitchline(&this->gap, this),
    crosshair(&this->gap, this),
    huddata(huddata)
{
    this->gap = 6;
    this->pitchcount = 5;
    this->degstep = 10;
    this->pen.setColor(Qt::green);
}

void HUD2IndicatorHorizon::updateGeometry(const QSize &size){
    int a = percent2pix_w(size, this->gap);

    // wings
    int x1 = size.width() / 2;
    int tmp = percent2pix_h(size, 1);
    hud2_clamp(tmp, 2, 10);
    pen.setWidth(tmp);
    hirizonleft.setLine(-x1, 0, -a, 0);
    horizonright.setLine(a, 0, x1, 0);
    
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
void HUD2IndicatorHorizon::drawpitchlines(QPainter *painter, qreal degstep, qreal pixstep){

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

void HUD2IndicatorHorizon::paint(QPainter *painter){

    qreal pitch = rad2deg(-huddata.pitch);
    qreal delta_y = pitch * (pixstep / degstep);
    qreal delta_x = tan(-huddata.roll) * delta_y;




    // sky and ground poligons
//    painter->save();
//    QPolygon sky_polygon(1);
//    sky_polygon[0] = QPoint(0, 0);
//    int tmp = round(painter->window().width() * tan(huddata.roll) / 2);
//    sky_polygon.putPoints(1, 1, 0, painter->window().height() / 2 + delta_y - tmp);
//    sky_polygon.putPoints(2, 1, painter->window().width(), painter->window().height() / 2 + delta_y + tmp);
//    sky_polygon.putPoints(3, 1, painter->window().width(), 0);
//    sky_polygon.putPoints(4, 1, painter->window().width(), 0);

//    painter->setBrush(QBrush(Qt::blue));
//    painter->setPen(QPen(Qt::blue));
//    painter->drawPolygon(sky_polygon);
//    painter->restore();

    painter->save();
    painter->translate(painter->window().center());
    int w = painter->window().width();
    int h = painter->window().height();
    QPointF pts[] = {
        QPointF(-w/2.0, -h/2.0), // up left
        QPointF(w/2.0, -h/2.0), // up right
        QPointF(w, h/2), // down right
        QPointF(0, h/2) // down left
    };

    QPointF p = QPointF(-delta_x, delta_y);
    p = rotatePoint(rad2deg(huddata.roll), p);
    // y = kx +b
    // k = tan()
    qreal k = tan(huddata.roll);
    qreal b = p.ry() - k * p.rx();
    pts[2] = QPointF(w/2.0, w/2.0 * k + b);
    pts[3] = QPointF(-w/2.0, -w/2.0 * k + b);


    painter->setBrush(QBrush(Qt::blue));
    painter->setPen(QPen(Qt::blue));
    painter->drawPolygon(pts, 4);
    painter->restore();






    // pitch and horizon lines
    painter->save();
    painter->translate(painter->window().center());
    crosshair.paint(painter);

    // now perform complex transfomation of painter
    QPoint center = painter->window().center();
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.translate(delta_x, delta_y);
    transform.rotate(rad2deg(huddata.roll));

    painter->setTransform(transform);

    // pitchlines
    this->drawpitchlines(painter, degstep, pixstep);

    // horizon lines
    painter->setPen(pen);
    painter->drawLine(hirizonleft);
    painter->drawLine(horizonright);

    painter->restore();
}

void HUD2IndicatorHorizon::setColor(QColor color){
    pen.setColor(color);
}
