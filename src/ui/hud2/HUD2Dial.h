#ifndef HUD2DIAL_H
#define HUD2DIAL_H

#include <QWidget>
#include <QPainter>

#include "HUD2Data.h"

class HUD2Dial : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Dial(QWidget *parent = 0,
                      int rscale = 10, int xscale = 80, int yscale = 80, int marks = 10,
                      int markStep = 1, int hands = 3);
    void paint(QPainter *painter, qreal value);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize *size);

private:
    int rscale;         // radius = h / rscale
    int r;
    int xscale;         // center x coordinate in percents (0..100)
    int x;
    int yscale;         // center y coordinate in percents (0..100)
    int y;
    int marks;          // count of marks with numbers
    int markStep;       // step between marks' values
    int hands;          // total number of hands

    QPen dialPen;       // dial circle
    QPen markPen;       // mark numbers
    QFont markFont;
    QPen scratchPen;    // scratches around dial
    QPen numberPen;     // main number
    QFont numberFont;

    QPen *handPens;     // array with pens for dial hands
    qreal *handScales;  // scales vor _values_ of hands
};

#endif // HUD2DIAL_H
