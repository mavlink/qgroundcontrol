
#include <QtGui>
#include "HUD2RenderGL.h"
#include "HUD2Painter.h"

HUD2RenderGL::HUD2RenderGL(HUD2Painter *hudpainter, QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
      hudpainter(hudpainter)
{
    setAutoFillBackground(false);
}

void HUD2RenderGL::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::black);
    painter.setRenderHint(QPainter::Antialiasing);
    hudpainter->paint(&painter);
}

void HUD2RenderGL::resizeEvent(QResizeEvent *event){
    hudpainter->updateGeometry(&event->size());
}
