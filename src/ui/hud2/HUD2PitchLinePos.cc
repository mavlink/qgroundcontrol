#include <QtGui>
#include "HUD2PitchLinePos.h"

HUD2PitchLinePos::HUD2PitchLinePos(int *gap, QWidget *parent) :
    QWidget(parent)
{
    this->gap = gap;
    this->huddata = huddata;
    this->updateGeometry(new QSize(640, 480));
}

void HUD2PitchLinePos::updateGeometry(const QSize *size){

    // Positive pitch indicator:
    //
    //      _______      _______
    //     |10                  |
    //

    const int wscale = 12;
    const int hscale = 35;
    const int hmin = 3;

    int w = size->width()  / wscale;
    int h = size->height() / hscale;
    if (h < hmin)
        h = hmin;

    int x1 = *gap/2;
    int x2 = *gap/2 + w;

    lines[0] = QLine(x1, 0,     x2, 0); // long right
    lines[1] = QLine(-x1, 0,    -x2, 0);// long left
    lines[2] = QLine(x2, 0,     x2, h); // short right
    lines[3] = QLine(-x2, 0,    -x2, h);// short left

    pen.setWidth(2);
}

void HUD2PitchLinePos::paint(QPainter *painter){
    painter->drawLines(lines, 4);
}

void HUD2PitchLinePos::setColor(QColor color){
    pen.setColor(color);
}

