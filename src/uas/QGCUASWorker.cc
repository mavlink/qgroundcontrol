#include "QGCUASWorker.h"

#include <QGC.h>
#include <QCoreApplication>
#include <QDebug>

QGCUASWorker::QGCUASWorker() : QThread(),
    _should_exit(false)
{
}

void QGCUASWorker::quit()
{
    _should_exit = true;
}

void QGCUASWorker::run()
{
    while(!_should_exit) {

        QCoreApplication::processEvents();
        QGC::SLEEP::msleep(2);
    }
}
