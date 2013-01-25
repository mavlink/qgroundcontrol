
#include <QtGui>
#include "HUD2RenderNative.h"
#include "HUD2Drawer.h"

HUD2RenderNative::HUD2RenderNative(HUD2Data &huddata, QWidget *parent)
    : QWidget(parent),
      huddrawer(huddata, this)
{
    QPalette p(palette());
    p.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(p);
    pixmap_static = new QPixmap(this->width(), this->height());
    render_static = new QPainter(pixmap_static);
}

void HUD2RenderNative::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(0, 0, *pixmap_static);
    huddrawer.paint_dynamic(&painter);
}

void HUD2RenderNative::resizeEvent(QResizeEvent *event){
    huddrawer.updateGeometry(event->size());

    delete render_static;
    delete pixmap_static;
    pixmap_static = new QPixmap(event->size());
    render_static = new QPainter(pixmap_static);
    render_static->setRenderHint(QPainter::Antialiasing);
    render_static->fillRect(pixmap_static->rect(), Qt::black);

    huddrawer.paint_static(render_static);
}

void HUD2RenderNative::paint(void){
    this->repaint();
}
