#ifndef MAV2DICON_H
#define MAV2DICON_H

#include "QGC2DIcon.h"

class MAV2DIcon : public QGC2DIcon
{
public:
    explicit MAV2DIcon(QGraphicsItem* parent = 0);

    QRectF boundingRect() const;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);

};

#endif // MAV2DICON_H
