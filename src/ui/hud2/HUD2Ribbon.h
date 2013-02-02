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

    qreal bigScratchLenStep; // step in percents of widget sizes
    qreal _big_pixstep;
    qreal bigScratchNumStep; // numerical value step
    int smallScratchCnt; // how many small scratches between 2 big. Can be 0.
    qreal _small_pixstep;
    qreal clipLen; // length of clipping rectangle

    QPen bigPen;
    QPen smallPen;
    QLineF _scratch_big;
    QLineF _scratch_small;
    QFont font;
};

#endif // HUD2RIBBON_H
