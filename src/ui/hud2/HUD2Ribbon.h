#ifndef HUD2RIBBON_H
#define HUD2RIBBON_H

#include <QWidget>
#include <QPen>
#include <QFont>

typedef enum {
    POSITION_LEFT = 0,
    POSITION_RIGHT = 1,
    POSITION_TOP = 2,
    POSITION_BOTTOM = 3
}screen_position;

class HUD2Ribbon : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Ribbon(screen_position position, QWidget *parent);
    void paint(QPainter *painter, float value);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

public slots:

private:
    void updateRibbon(const QSize &size, int gap, int len);
    void updateNumIndicator(const QSize &size, qreal num_w_percent, int fntsize, int len, int gap);
    screen_position position;
    bool opaqueNum;
    bool opaqueRibbon;

    qreal bigScratchLenStep; // step in percents of widget sizes
    qreal big_pixstep;
    int bigScratchValueStep; // numerical value step
    int stepsSmall; // how many small scratches between 2 big. Can be 0.
    int smallStepsTotal; // overall count of small scratches in ribbon.
    int stepsBig;
    qreal small_pixstep;
    QRect clipRect; // clipping rectangle

    QPen bigPen;
    QPen arrowPen;
    QPen smallPen;
    QLine *scratchBig;
    QLine *scratchSmall;
    QRect *labelRect;
    QFont labelFont;
    QPolygon numPoly;
};

#endif // HUD2RIBBON_H
