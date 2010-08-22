#ifndef WAYPOINT2DICON_H
#define WAYPOINT2DICON_H

#include <QGraphicsItem>
#include "QGC2DIcon.h"

class Waypoint2DIcon : public QGC2DIcon
{
public:
    explicit Waypoint2DIcon(QGraphicsItem* parent = 0);

    QRectF boundingRect() const;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);


};

#endif // WAYPOINT2DICON_H
