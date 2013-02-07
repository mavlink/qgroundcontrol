#include <QtGui>

#include "HUD2Ribbon.h"
#include "HUD2Math.h"

HUD2Ribbon::HUD2Ribbon(const float *value, bool mirrored, QWidget *parent) :
    QWidget(parent),
    value(value),
    mirrored(mirrored)
{
    rotated90 = false;
    opaqueNum = false;
    opaqueRibbon = false;

    bigScratchLenStep = 20.0;
    bigScratchValueStep = 1;
    stepsSmall = 4;
    stepsBig = 4;

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

    scratchBig = new QLine[1];
    scratchSmall = new QLine[1];
    labelRect = new QRect[1];
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
    int i = 0;
    qreal gap_percent = 6; // space between screen border and scratches
    qreal len_percent = 2; // length of big scratch
    qreal fontsize_percent = 2.0; // font size of labels on ribbon
    qreal num_w_percent = gap_percent * 1.2; // width of number indicator

    big_pixstep = percent2pix_hF(size, bigScratchLenStep);
    small_pixstep = big_pixstep / (stepsSmall + 1);

    int gap = percent2pix_w(size, gap_percent);
    int len = percent2pix_w(size, len_percent);
    hud2_clamp(len, 4, 20);

    // font size for labels
    int fntsize = percent2pix_d(size, fontsize_percent);
    hud2_clamp(fntsize, 7, 50);
    labelFont.setPixelSize(fntsize);

    // scratches
    int w_render;
    w_render = size.width();


    delete[] scratchBig;
    delete[] scratchSmall;
    delete[] labelRect;

    if (mirrored){
        // big scratches
        scratchBig = new QLine[stepsBig];
        scratchBig[0] = QLine(QPoint(w_render, 0), QPoint(w_render - len, 0));
        for (i=0; i<stepsBig; i++){
            scratchBig[i] = scratchBig[0];
            scratchBig[i].translate(0, round(big_pixstep * i));
        }

        // small scratches
        smallStepsCnt = (stepsBig + 2) * stepsSmall;
        int n = smallStepsCnt;

        scratchSmall = new QLine[n];
        scratchSmall[0] = QLine(QPoint(w_render - (len / 2), 0), QPoint(w_render - len, 0));
        scratchSmall[0].translate(0, round(-big_pixstep));

        for (i=0; i<n; i++){
            scratchSmall[i] = scratchSmall[0];
            scratchSmall[i].translate(0, round(small_pixstep * i));
            // TODO: skip rendering small scratch under the big one
        }

        // rectangles for labels
        labelRect = new QRect[stepsBig];
        labelRect[0] = QRect(w_render - gap - len, 0, gap, big_pixstep);
        labelRect[0].translate(0, -labelRect[0].height()/2);
        for (i=0; i<stepsBig; i++){
            labelRect[i] = labelRect[0];
            labelRect[i].translate(0, round(big_pixstep * i));
        }
    }
    else{
        scratchBig = new QLine[stepsBig];
        scratchSmall = new QLine[stepsBig*10];
        labelRect = new QRect[stepsBig];
//        scratch_big = QLineF(QPoint(0, 0), QPoint(len, 0));
//        scratch_small = QLineF(QPoint(len/2, 0), QPoint(len, 0));

//        labelRect = QRectF(len, 0, gap, big_pixstep);
//        labelRect.translate(0, -labelRect.height()/2);
    }

    // main number
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
        clipRect = QRect(w_render - gap - len, 0, w_clip,  big_pixstep * stepsBig);
    else
        clipRect = QRect(0, 0, w_clip,  big_pixstep * stepsBig);
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
    QPolygon _numPoly = numPoly;
    qreal v = *value;
    int i = 0;

    // translate starting point of ribbon
    qreal shift = (*value * big_pixstep) / bigScratchValueStep;
    shift = modulusF(shift, big_pixstep);
    shift = round(shift);

    painter->save();

    if (opaqueRibbon)
        painter->fillRect(clipRect, Qt::black);

    // arrow
    _numPoly.translate(0, big_pixstep * (stepsBig / 2));
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

    // translate painter to shift whole ribbon
    painter->translate(0, shift);

    // scratches big
    painter->setPen(bigPen);
    painter->drawLines(scratchBig, stepsBig);

    // scratches small
    painter->setPen(smallPen);
    painter->drawLines(scratchSmall, smallStepsCnt);

    // number labels
    painter->setFont(labelFont);
    v = v + stepsBig / 2;
    for (i=0; i<stepsBig; i++){
        //painter->fillRect(_labelRect, Qt::red);
        painter->drawText(labelRect[i], Qt::AlignCenter, QString::number((int)v));
        v -= bigScratchValueStep;
    }

    // make clean
    painter->restore();
}

void HUD2Ribbon::setColor(QColor color){
    Q_UNUSED(color);
}
