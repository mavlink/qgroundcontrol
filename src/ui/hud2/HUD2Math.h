#ifndef HUD2MATH_H
#define HUD2MATH_H

#include <math.h>
#include <QSize>
#include <QLine>

int percent2pix_h(const QSize *size, qreal percent);
int percent2pix_w(const QSize *size, qreal percent);
int percent2pix_d(const QSize *size, qreal percent);
qreal rad2deg(float rad);
qreal deg2rad(float deg);
QPoint rotatePoint(qreal phi, QPoint p);
QLine rotateLine(qreal phi, QLine line);

#endif // HUD2MATH_H
