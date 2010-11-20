#ifndef SLUGSVIDEOCAMCONTROL_H
#define SLUGSVIDEOCAMCONTROL_H

#include <QWidget>
#include <QMouseEvent>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

namespace Ui {
    class SlugsVideoCamControl;
}

class SlugsVideoCamControl : public QGraphicsView

{
    Q_OBJECT

public:
    explicit SlugsVideoCamControl(QWidget *parent = 0);
    ~SlugsVideoCamControl();

protected:
   // virtual void mousePressEvent(QMouseEvent* event);
    //virtual void mouseReleaseEvent(QMouseEvent* event);
    //void mouseMoveEvent(QMouseEvent* event);
     void mouseMoveEvent(QMouseEvent* event);
   // virtual void wheelEvent(QWheelEvent* event);
    //virtual void resizeEvent(QResizeEvent* event);




private:
    Ui::SlugsVideoCamControl *ui;
};

#endif // SLUGSVIDEOCAMCONTROL_H
