#ifndef HUD2RENDERGL_H
#define HUD2RENDERGL_H

#include <QGLWidget>
#include "HUD2Data.h"
#include "HUD2Drawer.h"

class HUD2Drawer;
QT_BEGIN_NAMESPACE
class QPaintEvent;
class QWidget;
QT_END_NAMESPACE

class HUD2RenderGL : public QGLWidget
{
    Q_OBJECT

public:
    HUD2RenderGL(HUD2Drawer *huddrawer, QWidget *parent);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void paint(void);
    void toggleAntialiasing(bool aa);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    HUD2Drawer *huddrawer;
    bool antiAliasing;
};

#endif /* HUD2RENDERGL_H */
