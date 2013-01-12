#include <QtGui>

#include "HUD2RenderOffscreen.h"
#include "HUD2Painter.h"

HUD2RenderOffscreen::HUD2RenderOffscreen(HUD2Painter *hudpainter, QWidget *parent)
    : QWidget(parent),
      hudpainter(hudpainter)
{
    QPalette p(palette());
    p.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(p);
}

void HUD2RenderOffscreen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    hudpainter->paint(&painter);
}

void HUD2RenderOffscreen::resizeEvent(QResizeEvent *event){
    hudpainter->updateGeometry(&event->size());
}
