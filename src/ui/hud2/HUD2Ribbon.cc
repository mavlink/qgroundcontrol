#include <QPainter>

#include "HUD2Ribbon.h"
#include "HUD2Math.h"

HUD2Ribbon::HUD2Ribbon(const float *value, bool mirrored, QWidget *parent) :
    QWidget(parent),
    value(value),
    mirrored(mirrored)
{
    mirrored = false;
    rotated90 = false;
    hideNegative = false;

    bigScratchLenStep = 10.0;
    bigScratchNumStep = 1;
    smallScratchCnt = 4;
    clipLen = 50;

    bigPen = QPen();
    bigPen.setColor(Qt::green);
    bigPen.setWidth(3);

    smallPen = QPen();
    smallPen.setColor(Qt::green);
    smallPen.setWidth(1);

    font = QFont();
    font.setPixelSize(15);
}

void HUD2Ribbon::updateGeometry(const QSize &size){

    _big_pixstep = percent2pix_hF(size, bigScratchLenStep);
    _small_pixstep = _big_pixstep / (smallScratchCnt + 1);

    qreal gap = 20; // space between screen border and scratches
    int len = 20; // length of big scratch

    if (mirrored){
        int w = size.width();
        _scratch_big = QLineF(QPointF(w - gap, 0), QPointF(w - len - gap, 0));
        _scratch_small = QLineF(QPointF(w - (len / 2) - gap, 0), QPointF(w - len - gap, 0));
    }
    else{
        _scratch_big = QLineF(QPoint(gap, 0), QPoint(gap + len, 0));
        _scratch_small = QLineF(QPoint(gap + len/2, 0), QPoint(gap + len, 0));
    }

}

void HUD2Ribbon::paint(QPainter *painter){
    QLineF scratchBig   = _scratch_big;
    QLineF scratchSmall = _scratch_small;

    // translate starting point of ribbon
    qreal shift = *value * bigScratchNumStep * _big_pixstep;
    scratchBig.translate(0, modulusF(shift, _big_pixstep));
    scratchSmall.translate(0, modulusF(shift, _big_pixstep));

    // draw scratches
    painter->save();

    for (int i=0; i<6; i++){
        painter->setPen(bigPen);
        painter->drawLine(scratchBig);
        scratchBig.translate(0, _big_pixstep);

        painter->setPen(smallPen);
        for (int n=0; n<smallScratchCnt; n++){
            scratchSmall.translate(0, _small_pixstep);
            painter->drawLine(scratchSmall);
        }
        // to skip rendering small scratch under the big one
        scratchSmall.translate(0, _small_pixstep);
    }

    painter->restore();
}

void HUD2Ribbon::setColor(QColor color){
    Q_UNUSED(color);
}
