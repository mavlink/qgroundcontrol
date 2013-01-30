#include <math.h>

#include <QtGui>

#include "HUD2Math.h"
#include "HUD2IndicatorRoll.h"

HUD2IndicatorRoll::HUD2IndicatorRoll(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    this->thickPen = QPen(Qt::green);
    this->thickPen.setWidth(3);

    this->thinPen = QPen(Qt::green);
    this->thinPen.setWidth(1);

    this->arrowPen = QPen(Qt::green);
    this->arrowPen.setWidth(2);
}

void HUD2IndicatorRoll::updateGeometry(const QSize &size){
    int tmp;

    // update pen widths
    tmp = percent2pix_h(size, 0.6);
    hud2_clamp(tmp, 2, 10);
    this->thickPen.setWidth(tmp);

    this->arrowPen.setWidth(tmp);

    tmp = percent2pix_h(size, 0.3);
    hud2_clamp(tmp, 1, 5);
    this->thinPen.setWidth(tmp);

    // update line lengths
    int thick_scratch_len = percent2pix_h(size, 2.5);
    hud2_clamp(thick_scratch_len, 4, 20);

    int thin_scratch_len = thick_scratch_len / 3;
    hud2_clamp(thin_scratch_len, 1, 10);

    int big_r = size.height() / 2;
    int small_r = big_r - thick_scratch_len;

    QLine thick_line = QLine(0, big_r, 0, small_r);
    QLine thin_line = QLine(0, small_r, 0, small_r + thin_scratch_len);

    // big scratches
    int phi_step = 30;
    int phi = -60;
    int i = 0;
    int n = 0;

    n = sizeof(thickLines) / sizeof(thickLines[0]);
    for (i=0; i<n; i++){
        thickLines[i] = rotateLine(phi, thick_line);
        phi += phi_step;
    }

    // small scratches
    phi_step = 10;
    phi = -70;
    n = sizeof(thinLines) / sizeof(thinLines[0]);
    i = 0;
    while (i < n){
        if ((phi % 30) != 0){
            thinLines[i] = rotateLine(phi, thin_line);
            i++;
        }
        phi += phi_step;
    }

    // arrow
    int a = percent2pix_h(size, 9);
    QPoint p0 = QPoint(0, small_r - arrowPen.width()); // top
    QPoint p1 = p0;
    QPoint p2 = p0;
    p1.rx() += a/4;
    p1.ry() -= a/4;
    p2.rx() -= a/4;
    p2.ry() -= a/4;
    arrowLines[0] = QLine(p0, p1);
    arrowLines[1] = QLine(p0, p2);
}

void HUD2IndicatorRoll::paint(QPainter *painter){
    int n = 0;

    painter->save();
    painter->translate(painter->window().center());

    painter->setPen(thickPen);
    n = sizeof(thickLines) / sizeof(thickLines[0]);
    painter->drawLines(thickLines, n);

    painter->setPen(thinPen);
    n = sizeof(thinLines) / sizeof(thinLines[0]);
    painter->drawLines(thinLines, n);

    n = sizeof(arrowLines) / sizeof(arrowLines[0]);
    painter->setPen(arrowPen);
    painter->rotate(rad2deg(huddata.roll));
    painter->drawLines(arrowLines, n);

    painter->restore();
}

void HUD2IndicatorRoll::setColor(QColor color){
    Q_UNUSED(color);
}
