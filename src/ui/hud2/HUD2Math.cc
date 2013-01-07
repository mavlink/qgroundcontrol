#include "HUD2Math.h"

int percent2pix_h(const QSize *size, qreal percent){
    return round(percent * size->height() / 100.0);
}

int percent2pix_w(const QSize *size, qreal percent){
    return round(percent * size->width() / 100.0);
}

int percent2pix_d(const QSize *size, qreal percent){
    qreal d; // diagonal
    d = sqrt(size->width() * size->width() + size->height() * size->height());
    return round(percent * d / 100.0);
}

qreal rad2deg(float rad){
    return rad * (180.0 / M_PI);
}

qreal deg2rad(float deg){
    return (deg * M_PI) / 180.0;
}

/**
 * @brief rotatePoint
 * @param phi           angle in degrees
 * @param p
 * @return
 */
QPoint rotatePoint(qreal phi, QPoint p){
    qreal x = p.x();
    qreal y = p.y();
    qreal x_, y_;

    x_ = round(x * cos(deg2rad(phi)) - y * sin(deg2rad(phi)));
    y_ = round(x * sin(deg2rad(phi)) + y * cos(deg2rad(phi)));

    return QPoint(x_, y_);
}

/**
 * @brief rotateLine
 * @param phi           angle in degrees
 * @param line
 * @return
 */
QLine rotateLine(qreal phi, QLine line){
    return QLine(rotatePoint(phi, line.p1()), rotatePoint(phi, line.p2()));
}

qreal wrap_360(qreal angle){
  if (angle > 360)
    angle -= 360;
  else if (angle < 0)
    angle += 360;
  return angle;
}
