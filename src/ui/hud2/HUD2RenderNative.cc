
#include <QtGui>
#include "HUD2RenderNative.h"
#include "HUD2Painter.h"

HUD2RenderNative::HUD2RenderNative(HUD2Painter *hudpainter, QWidget *parent)
    : QWidget(parent),
      hudpainter(hudpainter)
{
    QPalette p(palette());
    p.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(p);
}

void HUD2RenderNative::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    hudpainter->paint(&painter);
}

void HUD2RenderNative::resizeEvent(QResizeEvent *event){
    hudpainter->updateGeometry(&event->size());
}
