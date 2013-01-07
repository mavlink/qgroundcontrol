#ifndef HUD2HORIZONYAWINDICATOR_H
#define HUD2HORIZONYAWINDICATOR_H

#include <QWidget>
#include <QPen>

#include "HUD2Data.h"

class HUD2HorizonYaw : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2HorizonYaw(const HUD2Data *huddata, QWidget *parent = 0);
    void paint(QPainter *painter, QColor color);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize *size);
    
private:
    QPen thickPen;
    QPen thinPen;
    QPen arrowPen;
    QLineF thickLines[8];
    QLineF thinLines[sizeof(thickLines) / sizeof(thickLines[0])];
    QRect  rect;
    QLine  arrowLines[2];

    qreal scale_interval_pix;
    int scale_interval_deg;

    const HUD2Data *huddata;
};

#endif // HUD2HORIZONYAWINDICATOR_H
