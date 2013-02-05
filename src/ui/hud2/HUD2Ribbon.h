#ifndef HUD2RIBBON_H
#define HUD2RIBBON_H

#include <QWidget>
#include <QPen>
#include <QFont>

class HUD2Ribbon : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Ribbon(const float *value, bool mirrored, QWidget *parent);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

public slots:

private:
    const float *value;

    bool mirrored;
    bool rotated90;
    bool hideNegative; // do not draw negative values. Use it in speedometers.
    bool opaqueNum;
    bool opaqueRibbon;

    qreal bigScratchLenStep; // step in percents of widget sizes
    qreal big_pixstep;
    qreal bigScratchValueStep; // numerical value step
    int smallScratchCnt; // how many small scratches between 2 big. Can be 0.
    int stepsOnScreen;
    qreal small_pixstep;
    QRect clipRect; // clipping rectangle (automatically calculated)

    QPen bigPen;
    QPen medPen;
    QPen smallPen;
    QLineF scratch_big;
    QLineF scratch_small;
    QFont  labelFont;
    QRectF labelRect;
    QPolygon numPoly;
};

#endif // HUD2RIBBON_H
