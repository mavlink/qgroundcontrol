#ifndef HUD2HORIZONROLLINDICATOR_H
#define HUD2HORIZONROLLINDICATOR_H

#include <QWidget>
#include <QPen>

#include "HUD2Data.h"

class HUD2HorizonRoll : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2HorizonRoll(const qreal *gap, HUD2Data &huddata, QWidget *parent = 0);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

private:
    QPen thickPen;
    QPen thinPen;
    QPen arrowPen;
    QLine thickLines[5];
    QLine thinLines[10];
    QLine arrowLines[2];

    HUD2Data &huddata;
    const qreal *gap;
};

#endif // HUD2HORIZONROLLINDICATOR_H
