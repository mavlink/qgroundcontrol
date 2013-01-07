#include <QtGui>

#include "HUD2HorizonYaw.h"
#include "HUD2Math.h"

HUD2HorizonYaw::HUD2HorizonYaw(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    this->thickPen = QPen(Qt::green);
    this->thickPen.setWidth(3);

    this->thinPen = QPen(Qt::green);
    this->thinPen.setWidth(1);

    this->arrowPen = QPen(Qt::green);
    this->arrowPen.setWidth(2);

    scale_interval_deg = 5;
}

void HUD2HorizonYaw::updateGeometry(const QSize *size){
    int w = size->width() / 2;
    int h = size->height() / 2;
    int scratch_len = 20;
    int n = sizeof(thickLines) / sizeof(thickLines[0]);
    scale_interval_pix = w / (n + 2);
    qreal x;

    for(int i=0; i<n; i++){
        x = i * 2 * scale_interval_pix;
        thickLines[i] = QLineF(x, 0, x, scratch_len);
        thickLines[i].translate(0, -h);

        x += scale_interval_pix;
        thinLines[i] = QLineF(x, scratch_len/2, x, scratch_len);
        thinLines[i].translate(0, -h);
    }
    rect = QRect(QPoint(0,0), QSize(w, scratch_len));

    // arrow
    QPoint p0 = QPoint(0, 0); // top
    QPoint p1 = p0;
    QPoint p2 = p0;
    p1.rx() += 20;
    p1.ry() += 20;
    p2.rx() -= 20;
    p2.ry() += 20;
    arrowLines[0] = QLine(p0, p1);
    arrowLines[1] = QLine(p0, p2);

    arrowLines[0].translate(0, -h + scratch_len);
    arrowLines[1].translate(0, -h + scratch_len);
}

void HUD2HorizonYaw::paint(QPainter *painter, QColor color){
    painter->save();

    Q_UNUSED(color);
    int n = 0;

    n = sizeof(arrowLines) / sizeof(arrowLines[0]);
    painter->setPen(arrowPen);
    painter->drawLines(arrowLines, n);



    qreal yaw_deg = wrap_360(rad2deg(huddata->yaw));
    qreal delta_deg = yaw_deg / (2*scale_interval_deg);
    delta_deg = floor(delta_deg);
    delta_deg = yaw_deg - delta_deg * 2 * scale_interval_deg;

    qreal x = delta_deg * scale_interval_pix / scale_interval_deg;
    painter->translate(x - scale_interval_pix - rect.width()/2, 0);
    painter->fillRect(rect, Qt::red);
    int n_thick = sizeof(thickLines) / sizeof(thickLines[0]);
    int n_thin = sizeof(thinLines) / sizeof(thinLines[0]);

    painter->setPen(thickPen);
    if (delta_deg < scale_interval_deg)
        painter->drawLines(&thickLines[1], n_thick - 1);
    else
        painter->drawLines(thickLines, n_thick-1);

    painter->setPen(thinPen);
    if (delta_deg < scale_interval_deg)
        painter->drawLines(thinLines, n_thin-1);
    else
        painter->drawLines(thinLines, n_thin - 1);

    painter->restore();
}

void HUD2HorizonYaw::setColor(QColor color){
    Q_UNUSED(color);
}
