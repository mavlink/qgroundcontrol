#ifndef HUD2MATH_H
#define HUD2MATH_H

#include <math.h>
#include <QSize>
#include <QLine>

#define hud2_clamp(v, vmin, vmax){                                            \
  if (v <= vmin)                                                              \
    v = vmin;                                                                 \
  else if (v >= vmax)                                                         \
    v = vmax;                                                                 \
}

int percent2pix_h(QSize size, qreal percent);
int percent2pix_w(QSize size, qreal percent);
int percent2pix_d(QSize size, qreal percent);
qreal percent2pix_hF(QSize size, qreal percent);
qreal percent2pix_wF(QSize size, qreal percent);
qreal percent2pix_dF(QSize size, qreal percent);

qreal rad2deg(qreal rad);
qreal deg2rad(qreal deg);
QPoint rotatePoint(qreal phi, QPoint p);
QPointF rotatePoint(qreal phi, QPointF p);
QLine rotateLine(qreal phi, QLine line);
QLineF rotateLine(qreal phi, QLineF line);
qreal wrap_360(qreal angle);
int wrap_360(int angle);
qreal modulusF(qreal dividend, qreal divisor);

#endif // HUD2MATH_H
