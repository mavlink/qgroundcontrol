#ifndef HUD2CROSSHAIR_H
#define HUD2CROSSHAIR_H

#include <QWidget>
#include <QPen>
#include <QLine>

class HUD2HorizonCrosshair : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2HorizonCrosshair(const qreal *gap, QWidget *parent);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

private:
    const qreal *gap; /* space between right and left parts */
    QPen pen;
    QLine lines[3];
};

#endif
