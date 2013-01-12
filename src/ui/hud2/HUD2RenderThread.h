#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QThread>
#include <QTimer>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

#include "HUD2Painter.h"

class HUD2RenderThread : public QThread
{
    Q_OBJECT
public:
    explicit HUD2RenderThread(HUD2Painter &hudpainter, QObject *parent = 0);
    ~HUD2RenderThread();
    void paint(void);

signals:
    void renderedImage(const QImage &image);

protected:
    void run(void);

public slots:
    void updateGeometry(const QSize &size);

private:
    QMutex syncMutex; // for syncronization
    QMutex renderMutex; // for safe change of painter size
    QWaitCondition condition;
    HUD2Painter &hudpainter;
    QImage *image;
    QPainter *render;
    QTimer timer;
    bool abort;
};

#endif // RENDERTHREAD_H
