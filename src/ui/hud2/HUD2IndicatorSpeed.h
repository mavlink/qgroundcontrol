#ifndef HUD2INDICATORSPEED_H
#define HUD2INDICATORSPEED_H

#include <QWidget>

#include "HUD2Data.h"
#include "HUD2Ribbon.h"

class HUD2IndicatorSpeed : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorSpeed(const HUD2Data *huddata, QWidget *parent);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

private:
    const HUD2Data *huddata;
    HUD2Ribbon *ribbon;
};

#endif // HUD2INDICATORSPEED_H
