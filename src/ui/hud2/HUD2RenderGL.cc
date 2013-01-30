
#include <QtGui>
#include "HUD2RenderGL.h"
#include "HUD2Drawer.h"

HUD2RenderGL::HUD2RenderGL(HUD2Data &huddata, QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
      huddrawer(huddata, this)
{
    setAutoFillBackground(false);
    antiAliasing = true;
}

void HUD2RenderGL::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::black);
    painter.setRenderHint(QPainter::Antialiasing, antiAliasing);
    painter.setRenderHint(QPainter::TextAntialiasing, antiAliasing);
    huddrawer.paint(&painter);
}

void HUD2RenderGL::resizeEvent(QResizeEvent *event){
    huddrawer.updateGeometry(event->size());
}

void HUD2RenderGL::paint(void){
    this->repaint();
}

void HUD2RenderGL::toggleAntialiasing(bool aa){
    this->antiAliasing = aa;
    this->repaint();
}
