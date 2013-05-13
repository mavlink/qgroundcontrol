#ifndef HUD2RIBBON_H
#define HUD2RIBBON_H

#include <QWidget>
#include <QPen>
#include <QFont>

#include "HUD2Data.h"

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
    explicit HUD2Ribbon(screen_position position, bool wrap360, QString name, const HUD2Data *huddata, QWidget *parent);
    void paint(QPainter *painter);
    bool getOpacityNeedle(void){return opaqueNeedle;}
    bool getOpacityRibbon(void){return opaqueRibbon;}
    bool getEnabled(void){return enabled;}

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void setOpacityNeedle(bool op);
    void setOpacityRibbon(bool op);
    void setEnabled(bool checked);
    void updateGeometry(const QSize &size);
    void setValuePtr(int value_idx);

public slots:

private:
    const float *valuep;
    screen_position position;
    bool wrap360; // suitable for compass like device
    bool enabled;
    QString name;
    const HUD2Data *huddata;

    void updateRibbon(const QSize &size, int gap, int len);
    void updateNumIndicator(const QSize &size, qreal num_w_percent, int fntsize, int len, int gap);

    qreal bigScratchLenStep; // step in percents of widget sizes
    qreal big_pixstep;
    int bigScratchValueStep; // numerical value step
    int stepsSmall; // how many small scratches between 2 big. Can be 0.
    int small_steps_total; // overall count of small scratches in ribbon (internal use only).
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
    bool opaqueNeedle;
    bool opaqueRibbon;
};

#endif // HUD2RIBBON_H
