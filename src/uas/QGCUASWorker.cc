#include "QGCUASWorker.h"

#include <QGC.h>

QGCUASWorker::QGCUASWorker() : QThread()
{
}

void QGCUASWorker::run()
{
    QGC::SLEEP::msleep(100);
}
