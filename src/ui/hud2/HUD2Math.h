#ifndef HUD2MATH_H
#define HUD2MATH_H

#include <math.h>
#include <QSize>
#include <QLine>

#define clamp(v, vmin, vmax){                                                 \
  if (v <= vmin)                                                              \
    v = vmin;                                                                 \
  else if (v >= vmax)                                                         \
    v = vmax;                                                                 \
}

int percent2pix_h(const QSize *size, qreal percent);
int percent2pix_w(const QSize *size, qreal percent);
int percent2pix_d(const QSize *size, qreal percent);
qreal rad2deg(float rad);
qreal deg2rad(float deg);
QPoint rotatePoint(qreal phi, QPoint p);
QLine rotateLine(qreal phi, QLine line);
qreal wrap_360(qreal angle);
int wrap_360(int angle);
qreal modulusF(qreal dividend, qreal divisor);

#endif // HUD2MATH_H
