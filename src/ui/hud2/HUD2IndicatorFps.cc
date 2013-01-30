#include <QtGui>

#include "HUD2IndicatorFps.h"

HUD2IndicatorFps::HUD2IndicatorFps(QWidget *parent) :
    QWidget(parent)
{
    pen = QPen(Qt::green);
    pen.setWidth(1);

    font.setPixelSize(8);

    fps = 0;
    frames = 0;
    timer.setInterval(1000);
    connect(&timer, SIGNAL(timeout()), this, SLOT(calc()));
    timer.start();
}

void HUD2IndicatorFps::updateGeometry(const QSize &size){
    Q_UNUSED(size);
}

void HUD2IndicatorFps::paint(QPainter *painter){
    frames++;
    painter->save();
    painter->setPen(pen);
    painter->setFont(font);
    painter->drawText(0, painter->window().height(), QString::number(fps));
    painter->restore();
}

void HUD2IndicatorFps::setColor(QColor color){
    pen.setColor(color);
}

void HUD2IndicatorFps::calc(void){
    fps = frames;
    frames = 0;
}
