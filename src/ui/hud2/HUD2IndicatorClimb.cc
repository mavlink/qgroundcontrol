#include <QPainter>

#include "HUD2IndicatorClimb.h"

HUD2IndicatorClimb::HUD2IndicatorClimb(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    ribbon = new HUD2Ribbon(&huddata.alt, true, this);
}

void HUD2IndicatorClimb::updateGeometry(const QSize &size){
    ribbon->updateGeometry(size);
}

void HUD2IndicatorClimb::paint(QPainter *painter){
    painter->save();
    ribbon->paint(painter);
    painter->restore();
}

void HUD2IndicatorClimb::setColor(QColor color){
    Q_UNUSED(color);
}
