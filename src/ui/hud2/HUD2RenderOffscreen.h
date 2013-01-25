#ifndef HUD2RENDEROFFSCREEN_H
#define HUD2RENDEROFFSCREEN_H

#include <QWidget>
#include "HUD2Data.h"
#include "HUD2Drawer.h"
#include "HUD2RenderThread.h"

class HUD2Drawer;
QT_BEGIN_NAMESPACE
class QPaintEvent;
QT_END_NAMESPACE

class HUD2RenderOffscreen : public QWidget
{
    Q_OBJECT

public:
    HUD2RenderOffscreen(HUD2Data &huddata, QWidget *parent);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void paint(void);

private slots:
    void renderReady(const QImage &image);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    HUD2Drawer hudpainter;
    HUD2RenderThread renderThread;
    QPixmap pixmap;
};

#endif /* HUD2RENDEROFFSCREEN_H */
