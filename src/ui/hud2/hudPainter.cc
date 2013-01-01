#include <QColor>
#include "hudPainter.h"

hudPainter::hudPainter(HUD2data *data, QWidget *parent) :
    QWidget(parent),
    horizon(data)
{
    this->data = data;

    defaultColor = QColor(70, 255, 70);
    warningColor = Qt::yellow;
    criticalColor = Qt::red;
    infoColor = QColor(20, 200, 20);
    fuelColor = criticalColor;
}

void hudPainter::paint(QPainter *painter, QPaintEvent *event)
{
    Q_UNUSED(event);
    QRect hudrect = painter->viewport();
    painter->translate(hudrect.center());

    yaw.paint(painter, defaultColor);
    horizon.paint(painter, defaultColor);
}

void hudPainter::updateGeometry(const QSize *size){
    yaw.updatesize(size);
    horizon.updateGeometry(size);
}
