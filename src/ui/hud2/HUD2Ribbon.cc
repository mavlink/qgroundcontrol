#include <QtGui>

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
    bigScratchValueStep = 1;
    smallScratchCnt = 4;
    clipLen = 50;

    bigPen = QPen();
    bigPen.setColor(Qt::green);
    bigPen.setWidth(3);

    smallPen = QPen();
    smallPen.setColor(Qt::green);
    smallPen.setWidth(1);

    labelFont = QFont();
    labelFont.setPixelSize(15);
}

void HUD2Ribbon::updateGeometry(const QSize &size){
    qreal gap_percent = 6; // space between screen border and scratches
    qreal len_percent = 2; // length of big scratch
    qreal fontsize_percent = 2.0; // font size of labels on ribbon

    big_pixstep = percent2pix_hF(size, bigScratchLenStep);
    small_pixstep = big_pixstep / (smallScratchCnt + 1);

    qreal gap = percent2pix_wF(size, gap_percent);
    qreal len = percent2pix_wF(size, len_percent);
    hud2_clamp(len, 4, 20);

    // set scratch and lable rect
    if (mirrored){
        int w = size.width();
        scratch_big = QLineF(QPointF(w - gap, 0), QPointF(w - len - gap, 0));
        scratch_small = QLineF(QPointF(w - (len / 2) - gap, 0), QPointF(w - len - gap, 0));

        labelRect = QRectF(w - gap, 0, gap, big_pixstep);
        labelRect.translate(0, -labelRect.height()/2);


    }
    else{
        scratch_big = QLineF(QPoint(gap, 0), QPoint(gap + len, 0));
        scratch_small = QLineF(QPoint(gap + len/2, 0), QPoint(gap + len, 0));

        labelRect = QRectF(0, 0, gap, big_pixstep);
        labelRect.translate(0, -labelRect.height()/2);
    }

    // set font size
    int fntsize = percent2pix_d(size, fontsize_percent);
    hud2_clamp(fntsize, 8, 50);
    labelFont.setPixelSize(fntsize);
}

void HUD2Ribbon::paint(QPainter *painter){
    QRectF _labelRect = labelRect;
    QLineF _scratchBig   = scratch_big;
    QLineF _scratchSmall = scratch_small;

    // translate starting point of ribbon
    qreal shift = (*value * big_pixstep) / bigScratchValueStep;
    _scratchBig.translate(0, modulusF(shift, big_pixstep));
    _scratchSmall.translate(0, modulusF(shift, big_pixstep));
    _labelRect.translate(0, modulusF(shift, big_pixstep));

    // draw scratches
    painter->save();

    qreal v = *value;
    qDebug() << v;

    int stepsOnScreen = 7;
    int i = 0;

    // scratches big
    painter->setPen(bigPen);
    for (i=0; i<stepsOnScreen; i++){
        painter->drawLine(_scratchBig);
        _scratchBig.translate(0, big_pixstep);
    }

    // scratches small
    painter->setPen(smallPen);
    _scratchSmall.translate(0, -big_pixstep);
    for (i=0; i<(stepsOnScreen + 1); i++){
        for (int n=0; n<smallScratchCnt; n++){
            _scratchSmall.translate(0, small_pixstep);
            painter->drawLine(_scratchSmall);
        }
        // to skip rendering small scratch under the big one
        _scratchSmall.translate(0, small_pixstep);
    }

    // numbers
    painter->setFont(labelFont);
    for (i=0; i<stepsOnScreen; i++){
        painter->drawText(_labelRect, Qt::AlignCenter, QString::number((int)round(v)));
        v -= bigScratchValueStep;
        _labelRect.translate(0, big_pixstep);
    }

    // arrow
    int m = 10;
    QPolygon arrow = QPolygon(3);
    QPoint p0 = QPoint(100, 100);
    QPoint p1 = QPoint(p0.x() + m, p0.ry() - m);
    QPoint p2 = QPoint(p0.x() + m, p0.ry() + m);
    arrow.setPoint(0, p0);
    arrow.setPoint(1, p1);
    arrow.setPoint(2, p2);

    painter->drawPolygon(arrow);

    painter->restore();
}

void HUD2Ribbon::setColor(QColor color){
    Q_UNUSED(color);
}
