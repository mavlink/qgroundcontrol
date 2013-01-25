#include <QtGui>

#include "HUD2HorizonYaw.h"
#include "HUD2Math.h"

HUD2HorizonYaw::HUD2HorizonYaw(HUD2Data &huddata, QWidget *parent) :
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

void HUD2HorizonYaw::updateGeometry(const QSize &size){
/*
  000      010      020
   ||       ||       ||
   ||   I   ||   I   ||
(0;0)          /\

*/
    int i;
    qreal x;
    int scratch_len = percent2pix_h(size, 3);
    int text_size = (scratch_len * 3 ) / 2;
    clamp(text_size, SIZE_TEXT_MIN, 50);
    textFont.setPixelSize(text_size);
    mainRect.setWidth(size.width() / 3);
    mainRect.setHeight(scratch_len + text_size);
    clipRect = mainRect;
    clipRect.translate(-clipRect.width()/2, -clipRect.height());
    scale_interval_pix = mainRect.width() / 6;

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
    QPoint p0 = QPoint(0, 0); // top
    QPoint p1 = p0;
    QPoint p2 = p0;
    p1.rx() += scratch_len;
    p1.ry() += scratch_len;
    p2.rx() -= scratch_len;
    p2.ry() += scratch_len;
    arrowLines[0] = QLine(p0, p1);
    arrowLines[1] = QLine(p0, p2);
}

void HUD2HorizonYaw::paint_static(QPainter *painter){
    Q_UNUSED(painter);
    return;
}

void HUD2HorizonYaw::paint_dynamic(QPainter *painter){
    painter->save();

    int n_arrow = 0;
    int i;

    int y = painter->window().height()/2 - mainRect.height();
    painter->translate(0, -y);

    n_arrow = sizeof(arrowLines) / sizeof(arrowLines[0]);
    painter->setPen(arrowPen);
    painter->drawLines(arrowLines, n_arrow);
    painter->setPen(textPen);
    painter->setFont(textFont);

    qreal yaw_deg = wrap_360(rad2deg(huddata.yaw));
    //int deg = wrap_360(round(yaw_deg));
    //painter->drawText(QPoint(0, mainRect.height()), QString::number(deg));

    // number indicator
    painter->drawText(QPoint(0, mainRect.height()), QString::number(round(yaw_deg)));

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

void HUD2HorizonYaw::setColor(QColor color){
    Q_UNUSED(color);
}
