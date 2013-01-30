#include <QtGui>

#include "HUD2FpsIndicator.h"

HUD2FpsIndicator::HUD2FpsIndicator(QWidget *parent) :
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

void HUD2FpsIndicator::updateGeometry(const QSize &size){
    Q_UNUSED(size);
}

void HUD2FpsIndicator::paint(QPainter *painter){
    frames++;
    painter->save();
    painter->setPen(pen);
    painter->setFont(font);
    painter->translate(-painter->window().width() / 2, painter->window().height() / 2);
    painter->drawText(0, 0, QString::number(fps));
    painter->restore();
}

void HUD2FpsIndicator::setColor(QColor color){
    pen.setColor(color);
}

void HUD2FpsIndicator::calc(void){
    fps = frames;
    frames = 0;
}
