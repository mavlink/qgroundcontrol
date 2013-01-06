#ifndef HUD2CROSSHAIR_H
#define HUD2CROSSHAIR_H

#include <QWidget>
#include <QPen>
#include <QLine>

class HUD2Crosshair : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Crosshair(const int *gapscale, QWidget *parent);
    void paint(QPainter *painter, QColor color);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize *size);

private:
    const int *gapscale; /* space between right and left parts */
    QPen pen;
    QLine lines[3];
};

#endif
