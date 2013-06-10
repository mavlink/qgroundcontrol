#ifndef HUD2INDICATORCROSSHAIR_H
#define HUD2INDICATORCROSSHAIR_H

#include <QWidget>
#include <QPen>
#include <QLine>

class HUD2IndicatorHorizonCrosshair : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorHorizonCrosshair(const qreal *gap, QWidget *parent);
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

#endif /* HUD2INDICATORCROSSHAIR_H */
