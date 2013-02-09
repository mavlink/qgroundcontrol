#include <QPainter>

#include "HUD2IndicatorSpeed.h"

HUD2IndicatorSpeed::HUD2IndicatorSpeed(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    ribbon = new HUD2Ribbon(&huddata.alt, 1, false, this);
}

void HUD2IndicatorSpeed::updateGeometry(const QSize &size){
    ribbon->updateGeometry(size);
}

void HUD2IndicatorSpeed::paint(QPainter *painter){
    painter->save();
    ribbon->paint(painter);
    painter->restore();
}

void HUD2IndicatorSpeed::setColor(QColor color){
    Q_UNUSED(color);
}
