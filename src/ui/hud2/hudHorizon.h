#ifndef HUDHORIZON_H
#define HUDHORIZON_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "hudData.h"

class HudHorizon : public QWidget
{
    Q_OBJECT
public:
    explicit HudHorizon(HUD2data *data, QWidget *parent = 0);
    void paint(QPainter *painter, QColor color);
    void setColor(QColor color);

signals:
    
public slots:
    void updateGeometry(QSize *size);

private:
    void paintPitchLinePos(QString text, QPainter* painter);
    qreal rad2deg(float);
    int gap; /* space between right and left parts */
    const HUD2data *data;
    QPen pen;
    QLine left;
    QLine right;
};

#endif // HUDHORIZON_H
