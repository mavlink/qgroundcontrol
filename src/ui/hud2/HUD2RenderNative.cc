
#include <QtGui>
#include "HUD2RenderNative.h"
#include "HUD2Drawer.h"

HUD2RenderNative::HUD2RenderNative(HUD2Data &huddata, QWidget *parent)
    : QWidget(parent),
      huddrawer(huddata, this)
{
    antiAliasing = true;
    QPalette p(palette());
    p.setColor(QPalette::Background, Qt::black);
//    this->setAutoFillBackground(true);
//    this->setPalette(p);
}

void HUD2RenderNative::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, antiAliasing);
    painter.setRenderHint(QPainter::TextAntialiasing, antiAliasing);
    huddrawer.paint(&painter);
}

void HUD2RenderNative::resizeEvent(QResizeEvent *event){
    huddrawer.updateGeometry(event->size());
}

void HUD2RenderNative::paint(void){
    this->repaint();
}

void HUD2RenderNative::toggleAntialiasing(bool aa){
    this->antiAliasing = aa;
}
