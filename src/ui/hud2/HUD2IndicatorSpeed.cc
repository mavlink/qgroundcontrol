#include <QPainter>

#include "HUD2IndicatorSpeed.h"

HUD2IndicatorSpeed::HUD2IndicatorSpeed(const HUD2Data *huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    ribbon = new HUD2Ribbon(POSITION_LEFT, this, false);
}

void HUD2IndicatorSpeed::updateGeometry(const QSize &size){
    ribbon->updateGeometry(size);
}

void HUD2IndicatorSpeed::paint(QPainter *painter){
    painter->save();
    ribbon->paint(painter, huddata->alt);
    painter->restore();
}

void HUD2IndicatorSpeed::setColor(QColor color){
    Q_UNUSED(color);
}
