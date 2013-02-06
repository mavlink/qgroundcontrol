#include <QtGui>

#include "HUD2Ribbon.h"
#include "HUD2Math.h"

HUD2Ribbon::HUD2Ribbon(const float *value, bool mirrored, QWidget *parent) :
    QWidget(parent),
    value(value),
    mirrored(mirrored)
{
    rotated90 = false;
    hideNegative = false;
    opaqueNum = false;
    opaqueRibbon = false;

    bigScratchLenStep = 20.0;
    bigScratchValueStep = 1;
    smallScratchCnt = 4;
    stepsOnScreen = 4;

    bigPen = QPen();
    bigPen.setColor(Qt::green);
    bigPen.setWidth(3);

    medPen = QPen();
    medPen.setColor(Qt::green);
    medPen.setWidth(2);

    smallPen = QPen();
    smallPen.setColor(Qt::green);
    smallPen.setWidth(1);

    labelFont = QFont();
    labelFont.setPixelSize(15);
}

/**
 * @brief Constructs 5-point polygon for number indicator pointing to (0,0) coordinates
 *        <==|
 * @param w width of rectangular part
 * @param h height
 * @return
 */
static QPolygon numIndicator(int w, int h, bool mirrored){
    QPolygon poly = QPolygon(5);
    QPoint p = QPoint(0, 0);
    poly.setPoint(0, p);

    if (mirrored){
        p.rx() -= h/2;
        p.ry() -= h/2;
        poly.setPoint(1, p);

        p.rx() -= w;
        poly.setPoint(2, p);

        p.ry() += h;
        poly.setPoint(3, p);

        p.rx() += w;
        poly.setPoint(4, p);
    }
    else{
        p.rx() += h/2;
        p.ry() -= h/2;
        poly.setPoint(1, p);

        p.rx() += w;
        poly.setPoint(2, p);

        p.ry() += h;
        poly.setPoint(3, p);

        p.rx() -= w;
        poly.setPoint(4, p);
    }

    return poly;
}

void HUD2Ribbon::updateGeometry(const QSize &size){
    qreal gap_percent = 6; // space between screen border and scratches
    qreal len_percent = 2; // length of big scratch
    qreal fontsize_percent = 2.0; // font size of labels on ribbon
    qreal num_w_percent = gap_percent * 1.2; // width of number indicator

    big_pixstep = percent2pix_hF(size, bigScratchLenStep);
    small_pixstep = big_pixstep / (smallScratchCnt + 1);

    qreal gap = percent2pix_wF(size, gap_percent);
    qreal len = percent2pix_wF(size, len_percent);
    hud2_clamp(len, 4, 20);

    // font size for labels
    int fntsize = percent2pix_d(size, fontsize_percent);
    hud2_clamp(fntsize, 7, 50);
    labelFont.setPixelSize(fntsize);

    // scratches
    int w_render;
    w_render = size.width();

    if (mirrored){
        scratch_big = QLineF(QPointF(w_render, 0), QPointF(w_render - len, 0));
        scratch_small = QLineF(QPointF(w_render - (len / 2), 0), QPointF(w_render - len, 0));

        labelRect = QRectF(w_render - gap - len, 0, gap, big_pixstep);
        labelRect.translate(0, -labelRect.height()/2);
    }
    else{
        scratch_big = QLineF(QPoint(0, 0), QPoint(len, 0));
        scratch_small = QLineF(QPoint(len/2, 0), QPoint(len, 0));

        labelRect = QRectF(len, 0, gap, big_pixstep);
        labelRect.translate(0, -labelRect.height()/2);
    }

    // number
    int w_num, h_num;
    w_num = percent2pix_d(size, num_w_percent);
    hud2_clamp(w_num, 25, 500);
    if (fntsize > 12)
        h_num = fntsize + 8;
    else
        h_num = fntsize + 4;

    if (mirrored){
        numPoly = numIndicator(w_num, h_num, mirrored);
        numPoly.translate(w_render - len, 0);
    }
    else{
        numPoly = numIndicator(w_num, h_num, mirrored);
        numPoly.translate(len, 0);
    }

    // clip rectangle
    int w_clip = gap + len + 1;
    if (mirrored)
        clipRect = QRect(w_render - gap - len, 0, w_clip,  big_pixstep * stepsOnScreen);
    else
        clipRect = QRect(0, 0, w_clip,  big_pixstep * stepsOnScreen);
    clipRect.setTop(clipRect.top() + big_pixstep/2);
    clipRect.setBottom(clipRect.bottom() - big_pixstep/2);
}

// helper
static QString numStrVal(qreal value, int decimals){
    hud2_clamp(decimals, 0, 4);
    if (decimals == 0)
        return QString::number((int)round(value));
    else
        return QString::number(value, 'f', decimals);
}

// helper
static QRect numStrRect(QPolygon &poly, bool mirrored){
    QRect rect = poly.boundingRect();

    if (mirrored)
        rect.setLeft(rect.left() - rect.height() / 2);
    else
        rect.setRight(rect.right() + rect.height() / 2);

    return rect;
}

void HUD2Ribbon::paint(QPainter *painter){
    QRectF _labelRect = labelRect;
    QLineF _scratchBig = scratch_big;
    QLineF _scratchSmall = scratch_small;
    QPolygon _numPoly = numPoly;
    qreal v = *value;
    int i = 0;

    // translate starting point of ribbon
    qreal shift = (*value * big_pixstep) / bigScratchValueStep;
    _scratchBig.translate(0, modulusF(shift, big_pixstep));
    _scratchSmall.translate(0, modulusF(shift, big_pixstep));
    _labelRect.translate(0, modulusF(shift, big_pixstep));

    painter->save();

    if (opaqueRibbon)
        painter->fillRect(clipRect, Qt::black);

    // arrow
    _numPoly.translate(0, big_pixstep * (stepsOnScreen / 2));
    if (opaqueNum)
        painter->setBrush(Qt::black);
    painter->setPen(medPen);
    painter->drawPolygon(_numPoly);

    // text in arrow
    painter->setFont(labelFont);
    painter->drawText(numStrRect(_numPoly, mirrored), Qt::AlignCenter, numStrVal(*value, 2));

    // clipping area. Consist of long verical rectangle for ribbon and
    // small horisontal rectangle for number
    QRegion clipReg = QRegion(clipRect) - QRegion(_numPoly.boundingRect());
    painter->setClipRegion(clipReg);

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
    v = v + stepsOnScreen / 2;
    for (i=0; i<stepsOnScreen; i++){
        //painter->fillRect(_labelRect, Qt::red);
        painter->drawText(_labelRect, Qt::AlignCenter, QString::number((int)v));
        v -= bigScratchValueStep;
        _labelRect.translate(0, big_pixstep);
    }

    // make clean
    painter->restore();
}

void HUD2Ribbon::setColor(QColor color){
    Q_UNUSED(color);
}
