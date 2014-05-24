#ifndef QGCUASWORKER_H
#define QGCUASWORKER_H

#include <QThread>

class QGCUASWorker : public QThread
{
public:
    QGCUASWorker();

protected:
    void run();
};

#endif // QGCUASWORKER_H
