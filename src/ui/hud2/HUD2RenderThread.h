#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QThread>
#include <QTimer>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

#include "HUD2Data.h"

class HUD2RenderThread : public QThread
{
    Q_OBJECT
public:
    explicit HUD2RenderThread(const HUD2Data *huddata, QObject *parent = 0);
    ~HUD2RenderThread();
    void render(void);

signals:
    void renderedImage(const QImage &image);

protected:
    void run(void);

public slots:
    void updateGeometry(const QSize *size);

private:
    QMutex mutex;
    QWaitCondition condition;
    const HUD2Data *huddata;
    QImage image;
    QTimer timer;
    bool abort;
};

#endif // RENDERTHREAD_H
