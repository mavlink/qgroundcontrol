#include <QtGui>
#include "HUD2HorizonCrosshair.h"

HUD2HorizonCrosshair::HUD2HorizonCrosshair(const int *gapscale, QWidget *parent) :
    QWidget(parent),
    gapscale(gapscale)
{
    pen.setColor(Qt::green);
    pen.setWidth(2);
}

void HUD2HorizonCrosshair::updateGeometry(const QSize *size){

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

void HUD2HorizonCrosshair::paint(QPainter *painter, QColor color){
    Q_UNUSED(color)
    painter->save();
    painter->setPen(pen);
    painter->drawLines(lines, 3);
    painter->restore();
}

void HUD2HorizonCrosshair::setColor(QColor color){
    pen.setColor(color);
}
