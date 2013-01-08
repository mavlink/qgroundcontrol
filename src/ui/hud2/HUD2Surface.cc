
#include <QtGui>
#include "HUD2Surface.h"
#include "HUD2Painter.h"

HUD2PaintSurface::HUD2PaintSurface(HUD2Painter *hudpainter, QWidget *parent)
    : QWidget(parent),
      hudpainter(hudpainter)
{
    QPalette p(palette());
    p.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(p);
}

void HUD2PaintSurface::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    hudpainter->paint(&painter);
}

void HUD2PaintSurface::resizeEvent(QResizeEvent *event){
    hudpainter->updateGeometry(&event->size());
}
