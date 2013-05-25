
#include <QtGui>
#include "HUD2RenderNative.h"
#include "HUD2Drawer.h"

HUD2RenderNative::HUD2RenderNative(HUD2Drawer *huddrawer, QWidget *parent) :
    QWidget(parent),
    huddrawer(huddrawer),
    plt(palette())
{
    antiAliasing = true;
    plt.setColor(QPalette::Background, Qt::black);
}

void HUD2RenderNative::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    this->setAutoFillBackground(true);
    this->setPalette(plt);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, antiAliasing);
    painter.setRenderHint(QPainter::TextAntialiasing, antiAliasing);
    huddrawer->paint(&painter);
}

void HUD2RenderNative::resizeEvent(QResizeEvent *event){
    huddrawer->updateGeometry(event->size());
}

void HUD2RenderNative::paint(void){
    this->repaint();
}

void HUD2RenderNative::toggleAntialiasing(bool aa){
    this->antiAliasing = aa;
}
