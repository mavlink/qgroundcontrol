#ifndef HUD2INDICATORCOMPASS_H
#define HUD2INDICATORCOMPASS_H

#include <QWidget>

#include "HUD2Data.h"
#include "HUD2Ribbon.h"

class HUD2IndicatorCompass : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorCompass(HUD2Data &huddata, QWidget *parent);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);
    void setRibbonOpacity(bool opacity);
    void setNeedleOpacity(bool opacity);

private:
    HUD2Data &huddata;
    HUD2Ribbon *ribbon;
};

#endif // HUD2INDICATORCOMPASS_H
