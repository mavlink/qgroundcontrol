#ifndef HUD2DIAL_H
#define HUD2DIAL_H

#include <QWidget>
#include <QPainter>

#include "HUD2Data.h"

class HUD2Dial : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Dial(qreal r, qreal x, qreal y,
                      int marks, int markStep, int hands,
                      QPen *handPens, qreal *handScales,
                      QWidget *parent = 0);
    void paint(QPainter *painter, qreal value);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

private:
    qreal r; // radius
    int _r;
    qreal x; // center x coordinate in percents (0..100)
    int _x;
    qreal y; // center y coordinate in percents (0..100)
    int _y;
    int marks; // count of marks with numbers
    int markStep; // step between marks' values
    int hands; // total number of hands

    QPen  dialPen;       // dial circle
    QPen  markPen;       // mark numbers
    QFont markFont;
    QPen  scratchPen;    // scratches around dial
    QPen  numberPen;     // main number
    QFont numberFont;

    QPen  *handPens;     // array with pens for dial hands
    QLine *handLines;     // array with pens for dial hands
    qreal *handScales;  // scales array for _values_ of hands relative to full turn
    QRect *markRects;   // array of rectagulars containing marks
    QString *markStrings; // numbers converted to strings
};

#endif // HUD2DIAL_H
