#ifndef QGCUASWORKER_H
#define QGCUASWORKER_H

#include <QThread>

class QGCUASWorker : public QThread
{
public:
    QGCUASWorker();

public slots:
    void quit();

protected:
    void run();

    bool _should_exit;
};

#endif // QGCUASWORKER_H
