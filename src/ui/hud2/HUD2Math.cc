#include "HUD2Math.h"

int percent2pix_h(const QSize *size, qreal percent){
    return round(percent * size->height() / 100.0);
}

int percent2pix_w(const QSize *size, qreal percent){
    return round(percent * size->width() / 100.0);
}

int percent2pix_d(const QSize *size, qreal percent){
    qreal d;
    d = sqrt(size->width() * size->width() + size->height() * size->height());
    return round(percent * d / 100.0);
}

qreal rad2deg(float rad){
    return rad * (180.0 / M_PI);
}
