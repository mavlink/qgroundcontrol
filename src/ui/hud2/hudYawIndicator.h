#ifndef HUDYAWINDICATOR_H
#define HUDYAWINDICATOR_H

#include <QRect>
#include <QPainter>
#include <QPaintEvent>
#include <QPolygon>

class HudYawIndicator
{

public:
    HudYawIndicator();
    void updatesize(const QSize *size);
    void paint(QPainter *painter, QColor color);
    void setColor(QColor color);

protected:
    int scale; // indicator_height == widget_height / scale

private:
    QPen pen;
    QColor color;
    QPolygon poly;

};

#endif // HUDYAWINDICATOR_H
