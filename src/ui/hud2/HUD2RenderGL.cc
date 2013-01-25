
#include <QtGui>
#include "HUD2RenderGL.h"
#include "HUD2Drawer.h"

HUD2RenderGL::HUD2RenderGL(HUD2Data &huddata, QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
      hudpainter(huddata, this)
{
    setAutoFillBackground(false);
}

void HUD2RenderGL::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::black);
    painter.setRenderHint(QPainter::Antialiasing);
    hudpainter.paint(&painter);
}

void HUD2RenderGL::resizeEvent(QResizeEvent *event){
    hudpainter.updateGeometry(event->size());
}

void HUD2RenderGL::paint(void){
    this->repaint();
}
