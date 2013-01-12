#ifndef HUD2RENDERGL_H
#define HUD2RENDERGL_H

#include <QGLWidget>
#include "HUD2Data.h"
#include "HUD2Painter.h"

class HUD2Painter;
QT_BEGIN_NAMESPACE
class QPaintEvent;
class QWidget;
QT_END_NAMESPACE

class HUD2RenderGL : public QGLWidget
{
    Q_OBJECT

public:
    HUD2RenderGL(HUD2Data &huddata, QWidget *parent);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void paint(void);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    HUD2Painter hudpainter;
};

#endif /* HUD2RENDERGL_H */
