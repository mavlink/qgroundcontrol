#ifndef HUD2PITCHLINE_H
#define HUD2PITCHLINE_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"

#define SIZE_H_MIN      2
#define SIZE_TEXT_MIN   7

class HUD2HorizonPitch : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2HorizonPitch(const qreal *gap, QWidget *parent);
    void paint(QPainter *painter, int deg);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void updateGeometry(const QSize &size);
    void setColor(QColor color);

private:
    qreal size_w;
    qreal size_h;
    const qreal *gap; /* space between right and left parts */

    QRect update_geometry_lines_pos(int _gap, int w, int h);
    QRect update_geometry_lines_neg(int _gap, int w, int h);

    QPen  pen;
    QLine lines_pos[4];
    QLine lines_neg[8];

    QPen  textPen;
    QFont textFont;
    QRect textRectPos;
    QRect textRectNeg;

protected:

};

#endif // HUD2PITCHLINE_H
