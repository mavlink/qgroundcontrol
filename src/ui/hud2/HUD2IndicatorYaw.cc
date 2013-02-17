#include <QtGui>

#include "HUD2IndicatorYaw.h"
#include "HUD2Math.h"

HUD2IndicatorYaw::HUD2IndicatorYaw(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    this->thickPen = QPen(Qt::green);
    this->thickPen.setWidth(3);

    this->thinPen = QPen(Qt::green);
    this->thinPen.setWidth(1);

    this->arrowPen = QPen(Qt::green);
    this->arrowPen.setWidth(2);

    textPen = QPen(Qt::green);
    textFont = QFont();

    overlap = 8;
    scale_interval_deg = 5;
    thickLinesCnt = 360 / (2 * scale_interval_deg) + 2*overlap;
    thinLinesCnt  = 360 / (2 * scale_interval_deg) + 2*overlap;
    thickLines = new QLineF[thickLinesCnt];
    thinLines  = new QLineF[thinLinesCnt];
    textRects  = new QRect[thickLinesCnt];
    textStrings = new QString[thickLinesCnt];

    opaqueBackground = true;
}

void HUD2IndicatorYaw::updateGeometry(const QSize &size){
/*
  000      010      020
   ||       ||       ||
   ||   I   ||   I   ||
(0;0)          /\

*/
    int tmp;

    // update pen widths
    tmp = percent2pix_h(size, 0.6);
    hud2_clamp(tmp, 2, 10);
    this->thickPen.setWidth(tmp);

    tmp = percent2pix_h(size, 0.3);
    hud2_clamp(tmp, 1, 5);
    this->thinPen.setWidth(tmp);

    this->arrowPen.setWidth(tmp);

    // rect sizes
    int i;
    qreal x;
    int scratch_len = percent2pix_h(size, 3);
    int text_size = (scratch_len * 3 ) / 2;
    hud2_clamp(text_size, SIZE_TEXT_MIN, 50);
    textFont.setPixelSize(text_size);
    mainRect.setWidth(size.width() / 3);
    mainRect.setHeight(scratch_len + text_size);
    clipRect = mainRect;
    clipRect.translate(-clipRect.width()/2, -clipRect.height());
    scale_interval_pix = mainRect.width() / 6;

    // ribbon lines
    for(i=0; i<thickLinesCnt; i++){
        x = (i - overlap) * 2 * scale_interval_pix;
        thickLines[i] = QLineF(x, 0, x, -scratch_len);

        textRects[i]  = QRect(x, 0, x, -text_size);
        textRects[i].setWidth(2 * scale_interval_pix);
        textRects[i].translate(-textRects[i].width()/2, -scratch_len);

        int deg = (i - overlap) * 2 * scale_interval_deg;
        deg = wrap_360(deg);
        textStrings[i] = QString::number(deg);
    }
    for(i=0; i<thinLinesCnt; i++){
        x = i * 2 * scale_interval_pix + scale_interval_pix;
        x -= overlap * 2 * scale_interval_pix;
        thinLines[i] = QLineF(x, 0, x, -scratch_len/2);
    }

    // arrow
    QPoint p0 = QPoint(0, 0 + arrowPen.width()); // top
    QPoint p1 = p0;
    QPoint p2 = p0;
    p1.rx() += scratch_len;
    p1.ry() += scratch_len;
    p2.rx() -= scratch_len;
    p2.ry() += scratch_len;
    arrowLines[0] = QLine(p0, p1);
    arrowLines[1] = QLine(p0, p2);

    // rectangle for number indicator
    numRect = QRect(p0, QSize((scratch_len * 7) / 2, scratch_len * 2));
    numRect.translate(-numRect.width()/2, scratch_len);
}

void HUD2IndicatorYaw::paint(QPainter *painter){
    painter->save();
    painter->translate(painter->window().center());

    int n_arrow = 0;
    int i;

    int y = painter->window().height()/2 - mainRect.height();
    painter->translate(0, -y);

    n_arrow = sizeof(arrowLines) / sizeof(arrowLines[0]);
    painter->setPen(arrowPen);
    painter->drawLines(arrowLines, n_arrow);

    // number indicator
    if (opaqueBackground)
        painter->eraseRect(numRect);
    painter->drawRoundedRect(numRect, 1, 1);
    painter->setPen(textPen);
    painter->setFont(textFont);
    char str[4];
    qreal yaw_deg = wrap_360(rad2deg(huddata.yaw));
    snprintf(str, sizeof(str), "%03d", (int)round(yaw_deg));
    painter->drawText(numRect, Qt::AlignCenter, str);

    // ribbon compass
    qreal x = (yaw_deg * scale_interval_pix) / scale_interval_deg;

    if (opaqueBackground)
        painter->eraseRect(clipRect);
    painter->setClipRect(clipRect);

    painter->translate(-x, 0);

    painter->setPen(thickPen);
    painter->drawLines(thickLines, thickLinesCnt);

    painter->setPen(thinPen);
    painter->drawLines(thinLines, thinLinesCnt);

    painter->setPen(textPen);
    painter->setFont(textFont);
    for(i=0; i<thickLinesCnt; i++)
        painter->drawText(textRects[i], Qt::AlignCenter, textStrings[i]);

    painter->restore();
}

void HUD2IndicatorYaw::setColor(QColor color){
    Q_UNUSED(color);
}
