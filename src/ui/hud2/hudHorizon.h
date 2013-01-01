#ifndef HUDHORIZON_H
#define HUDHORIZON_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "hudData.h"
#include "HUD2PitchLinePos.h"

class HudHorizon : public QWidget
{
    Q_OBJECT
public:
    explicit HudHorizon(HUD2data *data, QWidget *parent = 0);
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
    const HUD2data *data;
    QPen pen;
    QLine left;
    QLine right;
};

#endif // HUDHORIZON_H
