#ifndef QGC2DICON_H
#define QGC2DICON_H

#include <QGraphicsItem>
#include <QPointF>

class QGC2DIcon : public QGraphicsItem
{
public:
    QGC2DIcon(QPointF localOriginInGlobalCoords, bool onlyLocal=false, QGraphicsItem* parent = 0);
    QGC2DIcon(bool onlyLocal=false, QGraphicsItem* parent = 0);
    explicit QGC2DIcon(QGraphicsItem* parent = 0);
    ~QGC2DIcon();

    QPointF getGlobalPosition();
    QPointF getLocalPosition();

    void setGlobalPosition(QPointF pos);
    void setLocalPosition(QPointF pos);
    void setLocalPosition(float x, float y);

    bool isLocal();
    virtual QRectF boundingRect() const = 0;
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) = 0;

signals:

public slots:

protected:
    QPointF localOriginInGlobalCoords;
    QPointF globalPosition;
    QPointF localPosition;
    bool local;

};

#endif // QGC2DICON_H
