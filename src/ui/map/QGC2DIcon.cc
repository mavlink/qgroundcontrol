#include "QGC2DIcon.h"

QGC2DIcon::QGC2DIcon(QPointF localOriginInGlobalCoords, bool onlyLocal, QGraphicsItem* parent) :
    QGraphicsItem(parent),
    localOriginInGlobalCoords(localOriginInGlobalCoords),
    local(onlyLocal)
{
}

QGC2DIcon::QGC2DIcon(bool onlyLocal, QGraphicsItem* parent) :
    QGraphicsItem(parent),
    localOriginInGlobalCoords(QPointF(0, 0)),
    local(onlyLocal)
{
}

QGC2DIcon::QGC2DIcon(QGraphicsItem* parent) :
        QGraphicsItem(parent),
        localOriginInGlobalCoords(QPointF(0, 0)),
        local(false)
{

}

QGC2DIcon::~QGC2DIcon()
{

}

QPointF QGC2DIcon::getGlobalPosition()
{

}

QPointF QGC2DIcon::getLocalPosition()
{

}

void QGC2DIcon::setGlobalPosition(QPointF pos)
{

}

void QGC2DIcon::setLocalPosition(QPointF pos)
{

}

void QGC2DIcon::setLocalPosition(float x, float y)
{

}

bool QGC2DIcon::isLocal()
{
    return local;
}
