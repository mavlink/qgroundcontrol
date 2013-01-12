#include <QtGui>

#include "HUD2RenderOffscreen.h"
#include "HUD2Painter.h"

HUD2RenderOffscreen::HUD2RenderOffscreen(HUD2Data &huddata, QWidget *parent)
    : QWidget(parent),
      hudpainter(huddata, this),
      renderThread(hudpainter, this)
{
    connect(&renderThread, SIGNAL(renderedImage(const QImage)), this, SLOT(renderReady(QImage)));
    this->renderThread.start();
}

void HUD2RenderOffscreen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    //painter.fillRect(event->rect(), Qt::black);
    painter.drawPixmap(0, 0, pixmap);
}

void HUD2RenderOffscreen::resizeEvent(QResizeEvent *event){
    renderThread.updateGeometry(event->size());
}

void HUD2RenderOffscreen::renderReady(const QImage &image){
    this->pixmap = QPixmap::fromImage(image);
    this->repaint();
}

void HUD2RenderOffscreen::paint(void){
    renderThread.paint();
}
