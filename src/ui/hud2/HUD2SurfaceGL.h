#ifndef HUDSURFACEGL_H
#define HUDSURFACEGL_H

#include <QGLWidget>
#include "HUD2Data.h"
#include "HUD2Painter.h"

class HUD2Painter;
QT_BEGIN_NAMESPACE
class QPaintEvent;
class QWidget;
QT_END_NAMESPACE

class HUD2PaintSurfaceGL : public QGLWidget
{
    Q_OBJECT

public:
    HUD2PaintSurfaceGL(HUD2Painter *hudpainter, QWidget *parent);

signals:
    void geometryChanged(const QSize *size);

public slots:

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    HUD2Painter *hudpainter;
};

#endif /* HUD2RENDERERGL_H */
