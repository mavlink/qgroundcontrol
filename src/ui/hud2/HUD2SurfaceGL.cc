
#include <QtGui>
#include "HUD2SurfaceGL.h"
#include "HUD2Painter.h"

HUD2PaintSurfaceGL::HUD2PaintSurfaceGL(HUD2Painter *hudpainter, QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
      hudpainter(hudpainter)
{
    setAutoFillBackground(false);
}

void HUD2PaintSurfaceGL::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), Qt::black);
    painter.setRenderHint(QPainter::Antialiasing);
    hudpainter->paint(&painter);
}

void HUD2PaintSurfaceGL::resizeEvent(QResizeEvent *event){
    hudpainter->updateGeometry(&event->size());
}
