#include <QtGui>

#include "HUD2RenderOffscreen.h"
#include "HUD2Drawer.h"

HUD2RenderOffscreen::HUD2RenderOffscreen(HUD2Data &huddata, QWidget *parent)
    : QWidget(parent),
      hudpainter(huddata, this),
      renderThread(hudpainter, this)
{
    connect(&renderThread, SIGNAL(renderedImage(const QImage)), this, SLOT(renderReady(QImage)));
    renderThread.start(QThread::LowPriority);
}

void HUD2RenderOffscreen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
}

void HUD2RenderOffscreen::resizeEvent(QResizeEvent *event){
    renderThread.updateGeometry(event->size());
    renderThread.paint();
}

void HUD2RenderOffscreen::renderReady(const QImage &image){
    pixmap = QPixmap::fromImage(image);
    repaint();
}

void HUD2RenderOffscreen::paint(void){
    renderThread.paint();
}
