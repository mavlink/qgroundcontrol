#include <QPainter>

#include "HUD2IndicatorClimb.h"
#include "HUD2Math.h"

HUD2IndicatorClimb::HUD2IndicatorClimb(HUD2Data &huddata, QWidget *parent) :
    QWidget(parent),
    huddata(huddata)
{
    ribbon = new HUD2Ribbon(POSITION_RIGHT, this);
}

void HUD2IndicatorClimb::updateGeometry(const QSize &size){
    ribbon->updateGeometry(size);
}

void HUD2IndicatorClimb::paint(QPainter *painter){
    painter->save();
    ribbon->paint(painter, rad2deg(huddata.roll));
    painter->restore();
}

void HUD2IndicatorClimb::setColor(QColor color){
    Q_UNUSED(color);
}
