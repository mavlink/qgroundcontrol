#ifndef HUD2MATH_H
#define HUD2MATH_H

#include <math.h>
#include <QSize>

int percent2pix_h(const QSize *size, qreal percent);
int percent2pix_w(const QSize *size, qreal percent);
int percent2pix_d(const QSize *size, qreal percent);
qreal rad2deg(float rad);

#endif // HUD2MATH_H
