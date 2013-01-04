#include <QtGui>
#include "HUD2Crosshair.h"

HUD2Crosshair::HUD2Crosshair(const int *gapscale, QWidget *parent) :
    QWidget(parent),
    gapscale(gapscale)
{
}

void HUD2Crosshair::updateGeometry(const QSize *size){

    int gap = size->width() / *gapscale;
    int minigap = gap/5;

    // left
    lines[0] = QLine(-gap/2, 0, 0, 0);
    lines[0].translate(-minigap, 0);

    // right
    lines[1] = QLine(0, 0, gap/2, 0);
    lines[1].translate(minigap, 0);

    // up
    lines[2] = QLine(0, -gap/2, 0, 0);
    lines[2].translate(0, -minigap);
}

void HUD2Crosshair::paint(QPainter *painter, QColor color){
    Q_UNUSED(color)
    painter->drawLines(lines, 3);
}

void HUD2Crosshair::setColor(QColor color){
    pen.setColor(color);
}
