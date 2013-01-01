#ifndef HUDHORIZON_H
#define HUDHORIZON_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"
#include "HUD2PitchLinePos.h"

class HUD2Horizon : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Horizon(HUD2data *huddata, QWidget *parent = 0);
    void paint(QPainter *painter, QColor color);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void updateColor(QColor color);
    void updateGeometry(const QSize *size);

private:
    HUD2PitchLinePos pitchlinepos;
    qreal rad2deg(float);
    int gap; /* space between right and left parts */
    const HUD2data *huddata;
    QPen pen;
    QLine left;
    QLine right;
};

#endif // HUDHORIZON_H
