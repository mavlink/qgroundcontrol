#include <QtGui>
#include "hudHorizon.h"

HudHorizon::HudHorizon(HUD2data *data, QWidget *parent) :
    QWidget(parent)
{
    this->data = data;
    gap = 0;
    pen.setWidth(6);
}

void HudHorizon::updateGeometry(QSize *size){
    int x1 = size->width();

    gap = x1 / 10;
    left.setLine(-x1, 0, -gap/2, 0);
    right.setLine(gap/2, 0, x1, 0);
}

void HudHorizon::paint(QPainter *painter, QColor color){
    painter->save();
    painter->rotate(-rad2deg(data->roll));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawLine(left);
    painter->drawLine(right);
    paintPitchLinePos("-20", painter);
    painter->restore();
}

void HudHorizon::setColor(QColor color){
    pen.setColor(color);
}

qreal HudHorizon::rad2deg(float rad){
    return rad * (180.0 / M_PI);
}

void HudHorizon::paintPitchLinePos(QString text, QPainter* painter)
{
    // Positive pitch indicator:
    //
    //      _______      _______
    //     |10                  |
    //

    const int w = 50;
    const int h = w / 12;
    const int lineWidth = 2;
    const float textSize = h * 1.6f;

    // Left vertical line
    painter->drawLine(-(gap/2 + w), -20, -gap/2, -20);
//    // Left horizontal line
//    drawLine(refPosX-pitchWidth/2.0f, refPosY, refPosX-pitchGap/2.0f, refPosY, lineWidth, defaultColor, painter);
//    // Text left
//    paintText(text, defaultColor, textSize, refPosX-pitchWidth/2.0 + 0.75f, refPosY + pitchHeight - 1.3f, painter);

//    // Right vertical line
//    drawLine(refPosX+pitchWidth/2.0f, refPosY, refPosX+pitchWidth/2.0f, refPosY+pitchHeight, lineWidth, defaultColor, painter);
//    // Right horizontal line
//    drawLine(refPosX+pitchWidth/2.0f, refPosY, refPosX+pitchGap/2.0f, refPosY, lineWidth, defaultColor, painter);
}
