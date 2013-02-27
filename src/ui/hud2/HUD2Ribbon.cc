#include <QtGui>

#include "HUD2Ribbon.h"
#include "HUD2Math.h"

HUD2Ribbon::HUD2Ribbon(screen_position position, QWidget *parent) :
    QWidget(parent),
    position(position)
{
    opaqueNum = false;
    opaqueRibbon = false;

    bigScratchLenStep = 15.0;
    bigScratchValueStep = 10;
    stepsSmall = 4;
    stepsBig = 4;
    if ((stepsBig < 2) || ((stepsBig % 2) != 0))
        qFatal("Ribbon's stepsBig value must be even AND more than 0");

    bigPen = QPen();
    bigPen.setColor(Qt::green);
    bigPen.setWidth(3);

    arrowPen = QPen();
    arrowPen.setColor(Qt::green);
    arrowPen.setWidth(2);

    smallPen = QPen();
    smallPen.setColor(Qt::green);
    smallPen.setWidth(1);

    labelFont = QFont();
    labelFont.setPixelSize(15);

    scratchBig = new QLine[1];
    scratchSmall = new QLine[1];
    labelRect = new QRect[1];
}

#define ribbonVertical() (((position) == POSITION_RIGHT) || ((position) == POSITION_LEFT))

/**
 * @brief Constructs 5-point polygon for number indicator pointing to (0,0) coordinates
 *        --------
 *      /        |
 *      \        |
 *        --------
 */
void HUD2Ribbon::updateNumIndicator(const QSize &size, qreal num_w_percent, int fntsize, int len, int gap){
    QPolygon poly;
    QPoint p;

    int w, h;
    w = percent2pix_d(size, num_w_percent);
    hud2_clamp(w, 25, 500);
    if (fntsize > 12)
        h = fntsize + 8;
    else
        h = fntsize + 4;

    switch (position){
    case POSITION_RIGHT:
        poly = QPolygon(5);
        p = QPoint(0, 0);
        poly.setPoint(0, p);

        p.rx() -= h/2;
        p.ry() -= h/2;
        poly.setPoint(1, p);

        p.rx() -= w;
        poly.setPoint(2, p);

        p.ry() += h;
        poly.setPoint(3, p);

        p.rx() += w;
        poly.setPoint(4, p);
        poly.translate(size.width() - len - gap - 2, 0);
        break;

    case POSITION_LEFT:
        poly = QPolygon(5);
        p = QPoint(0, 0);
        poly.setPoint(0, p);

        p.rx() += h/2;
        p.ry() -= h/2;
        poly.setPoint(1, p);

        p.rx() += w;
        poly.setPoint(2, p);

        p.ry() += h;
        poly.setPoint(3, p);

        p.rx() -= w;
        poly.setPoint(4, p);
        poly.translate(len + gap + 2, 0);
        break;

    case POSITION_TOP:
        poly = QPolygon(7);
        p = QPoint(0, 0);
        poly.setPoint(0, p);

        p.rx() += h/2;
        p.ry() += h/2;
        poly.setPoint(1, p);

        p.rx() += w/2 - h/2;
        poly.setPoint(2, p);

        p.ry() += h;
        poly.setPoint(3, p);

        p.rx() -= w;
        poly.setPoint(4, p);

        p.ry() -= h;
        poly.setPoint(5, p);

        p.rx() += w/2 - h/2;
        poly.setPoint(6, p);

        poly.translate(0, len + 2 + gap/2);
        break;

    default:
        qFatal("unhandled case");
    }

    numPoly = poly;
}


void HUD2Ribbon::updateRibbon(const QSize &size, int gap, int len){
    qreal shift;
    int i = 0;
    int w_render, h_render;
    w_render = size.width();
    h_render = size.height();

    // scratches
    smallStepsTotal = (stepsBig + 1) * stepsSmall;
    delete[] scratchBig;
    delete[] scratchSmall;
    delete[] labelRect;
    scratchBig = new QLine[stepsBig];
    scratchSmall = new QLine[smallStepsTotal];
    labelRect = new QRect[stepsBig];

    switch(position){
    case POSITION_RIGHT:
        scratchBig[0] = QLine(QPoint(w_render - gap, 0), QPoint(w_render - len - gap, 0));
        scratchSmall[0] = QLine(QPoint(w_render - (len / 2) - gap, 0), QPoint(w_render - len - gap, 0));
        labelRect[0] = QRect(w_render - gap, 0, gap, big_pixstep);
        break;
    case POSITION_LEFT:
        scratchBig[0] = QLine(QPoint(gap, 0), QPoint(gap + len, 0));
        scratchSmall[0] = QLine(QPoint(gap + len / 2, 0), QPoint(gap + len, 0));
        labelRect[0] = QRect(0, 0, gap, big_pixstep);
        break;
    case POSITION_TOP:
        scratchBig[0] = QLine(QPoint(0, gap/2), QPoint(0, len + gap/2));
        scratchSmall[0] = QLine(QPoint(0, len/2 + gap/2), QPoint(0, len + gap/2));
        //( int x, int y, int width, int height )
        labelRect[0] = QRect(0, 0, big_pixstep, gap/2);
        break;

    default:
        qFatal("unhandled case");
    }


    // big scratches
    shift = 0;
    for (i=1; i<stepsBig; i++){
        shift += big_pixstep;
        scratchBig[i] = scratchBig[0];
        if (ribbonVertical())
            scratchBig[i].translate(0, round(shift));
        else
            scratchBig[i].translate(round(shift), 0);
    }

    // small scratches
    shift = 0;
    // start _before_ first big scratch
    if (ribbonVertical())
        scratchSmall[0].translate(0, round(-big_pixstep));
    else
        scratchSmall[0].translate(round(-big_pixstep), 0);
    for (i=1; i<smallStepsTotal; i++){
        shift += small_pixstep;
        scratchSmall[i] = scratchSmall[0];
        if (ribbonVertical())
            scratchSmall[i].translate(0, round(shift));
        else
            scratchSmall[i].translate(round(shift), 0);
        // to skip rendering small scratch under the big one
        if (i % stepsSmall == 0)
            shift += small_pixstep;
    }

    // rectangles for labels
    shift = 0;
    if (ribbonVertical())
        labelRect[0].translate(0, -labelRect[0].height()/2);
    else
        labelRect[0].translate(-labelRect[0].width()/2, 0);

    for (i=1; i<stepsBig; i++){
        shift += big_pixstep;
        labelRect[i] = labelRect[0];
        if (ribbonVertical())
            labelRect[i].translate(0, round(shift));
        else
            labelRect[i].translate(round(shift), 0);
    }
}

void HUD2Ribbon::updateGeometry(const QSize &size){
    qreal gap_percent = 6;
    qreal len_percent = 2; // length of big scratch
    qreal fontsize_percent = 2.0; // font size of labels on ribbon
    qreal num_w_percent = gap_percent * 1.2; // width of number indicator

    if (ribbonVertical())
        big_pixstep = percent2pix_hF(size, bigScratchLenStep);
    else
        big_pixstep = percent2pix_wF(size, bigScratchLenStep);
    small_pixstep = big_pixstep / (stepsSmall + 1);

    int gap = percent2pix_w(size, gap_percent);
    int len = percent2pix_w(size, len_percent);
    hud2_clamp(len, 4, 20);

    // font size for labels
    int fntsize = percent2pix_d(size, fontsize_percent);
    hud2_clamp(fntsize, 7, 50);
    labelFont.setPixelSize(fntsize);

    // generate scratches and rectangles for ribbon
    updateRibbon(size, gap, len);

    // polygon for main number indicator
    updateNumIndicator(size, num_w_percent, fntsize, len, gap);

    // clip rectangle
    int w_clip = gap + len + 1;
    switch(position){
    case POSITION_RIGHT:
        clipRect = QRect(size.width() - gap - len, 0, w_clip, big_pixstep * stepsBig);
        break;
    case POSITION_LEFT:
        clipRect = QRect(0, 0, w_clip,  big_pixstep * stepsBig);
        break;
    case POSITION_TOP:
        clipRect = QRect(0, 0, big_pixstep * stepsBig, w_clip);
        break;
    case POSITION_BOTTOM:
        clipRect = QRect(0, size.height() - gap - len, big_pixstep * stepsBig, w_clip);
        break;
    default:
        qFatal("unhandled case");
    }
    if (ribbonVertical()){
        clipRect.setTop(clipRect.top() + big_pixstep/2);
        clipRect.setBottom(clipRect.bottom() - big_pixstep/2);
    }
    else{
        clipRect.setLeft(clipRect.left() + big_pixstep/2);
        clipRect.setRight(clipRect.right() - big_pixstep/2);
    }
}

// helper
static QString num_str_val(qreal value, int decimals){
    hud2_clamp(decimals, 0, 4);
    if (decimals == 0)
        return QString::number((int)round(value));
    else
        return QString::number(value, 'f', decimals);
}

// helper
static QRect num_str_rect(QPolygon &poly, screen_position position){
    QRect rect = poly.boundingRect();
    switch(position){
    case POSITION_RIGHT:
        rect.setLeft(rect.left() - rect.height() / 2);
        break;
    case POSITION_LEFT:
        rect.setRight(rect.right() + rect.height() / 2);
        break;
    case POSITION_TOP:
        rect.setTop(rect.top() + rect.height() / 2);
        break;
    case POSITION_BOTTOM:
        rect.setBottom(rect.bottom() - rect.height() / 2);
        break;
    default:
        qFatal("unhandled case");
    }
    return rect;
}

void HUD2Ribbon::paint(QPainter *painter, float value){
    QPolygon _numPoly = numPoly;
    qreal v = value;
    int i = 0;

    // calculate shift for starting point of ribbon
    qreal shift = (v * big_pixstep) / bigScratchValueStep;
    shift = modulusF(shift, big_pixstep);
    shift = round(shift);

    painter->save();

    if (!ribbonVertical()){
        painter->translate(painter->window().width()/2 - big_pixstep * (stepsBig / 2), 0);
    }

    if (opaqueRibbon)
        painter->fillRect(clipRect, Qt::black);

    // polygon for main number indicator
    if (ribbonVertical())
        _numPoly.translate(0, big_pixstep * (stepsBig / 2));
    else
        _numPoly.translate(big_pixstep * (stepsBig / 2), 0);

    if (opaqueNum)
        painter->setBrush(Qt::black);
    painter->setPen(arrowPen);
    painter->drawPolygon(_numPoly);

    // text in indicator
    painter->setFont(labelFont);
    painter->drawText(num_str_rect(_numPoly, position), Qt::AlignCenter, num_str_val(v, 2));

    // clipping area
    painter->setClipRegion(clipRect);

    // translate painter to shift whole ribbon
    // for antialiased painter we add halpixel shift to prevent bluring of lines
    if (ribbonVertical()){
        if (painter->Antialiasing)
            painter->translate(0, shift + 0.5);
        else
            painter->translate(0, shift);
    }
    else{
        if (painter->Antialiasing)
            painter->translate(shift + 0.75, 0);
        else
            painter->translate(shift, 0);
    }

    // scratches big
    painter->setPen(bigPen);
    painter->drawLines(scratchBig, stepsBig);

    // scratches small
    painter->setPen(smallPen);
    painter->drawLines(scratchSmall, smallStepsTotal);

    // number labels
    painter->setFont(labelFont);
    int v_int = int(v);
    v_int /= bigScratchValueStep;
    if (v < 0)
        v_int -= 1;
    v_int *= bigScratchValueStep;
    v_int += (stepsBig * bigScratchValueStep) / 2;
    for (i=0; i<stepsBig; i++){
        painter->drawText(labelRect[i], Qt::AlignCenter, QString::number(v_int));
        v_int -= bigScratchValueStep;
    }

    // make clean
    painter->restore();
}

void HUD2Ribbon::setColor(QColor color){
    Q_UNUSED(color);
}
