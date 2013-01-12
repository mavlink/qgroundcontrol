#include <QDebug>

#include "HUD2RenderThread.h"

HUD2RenderThread::HUD2RenderThread(HUD2Painter &hudpainter, QObject *parent) :
    QThread(parent),
    hudpainter(hudpainter),
    image(QImage(640, 480, QImage::Format_ARGB32_Premultiplied)),
    render(&image)
{
    abort = false;
    render.setRenderHint(QPainter::Antialiasing);

    //mutex.unlock();
    //image = QImage(512, 512, QImage::Format_ARGB32_Premultiplied);
}

HUD2RenderThread::~HUD2RenderThread(){
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
    qDebug() << "RenderThread: destroyed";
}

void HUD2RenderThread::run(void){
    qDebug() << "RenderThread: run";

    while (!abort){
        condition.wait(&mutex);

        render.fillRect(image.rect(), Qt::black);
        hudpainter.paint(&render);

        emit  renderedImage(image);
    }

    qDebug() << "RenderThread: quit";
    quit();
}

void HUD2RenderThread::paint(void){

    if (!isRunning())
        start(LowPriority);
    else{
        mutex.lock();
        condition.wakeOne();
        mutex.unlock();
    }
}

void HUD2RenderThread::updateGeometry(const QSize &size){
    hudpainter.updateGeometry(size);
}
