#include <QDebug>

#include "HUD2RenderThread.h"

HUD2RenderThread::HUD2RenderThread(const HUD2Data *huddata, QObject *parent) :
    QThread(parent),
    huddata(huddata),
    image(QImage(1024, 512, QImage::Format_ARGB32_Premultiplied))
{
    abort = false;
    //mutex.unlock();
}

HUD2RenderThread::~HUD2RenderThread(){
    qDebug() << "RenderThread: destructor";
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}

/**
 * @brief HUD2RenderThread::run
 */
void HUD2RenderThread::run(void){
    qDebug() << "RenderThread: run";

    while (!abort){
        qDebug() << "RenderThread: run cycle" << huddata->roll;
        condition.wait(&mutex);
        emit  renderedImage(image);
    }

    quit();
}

/**
 * @brief HUD2RenderThread::render
 */
void HUD2RenderThread::render(void){
    qDebug() << "RenderThread: render";

    if (!isRunning())
        start(LowPriority);
    else{
        //mutex.lock();
        condition.wakeOne();
        //mutex.unlock();
    }
}

/**
 * @brief HUD2RenderThread::updateGeometry
 * @param size
 */
void HUD2RenderThread::updateGeometry(const QSize *size){
    qDebug() << "RenderThread: updateGeometry" << size;
}
