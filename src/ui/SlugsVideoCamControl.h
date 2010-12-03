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

class SlugsVideoCamControl : public QWidget

{
    Q_OBJECT

public:
    explicit SlugsVideoCamControl(QWidget *parent = 0);
    ~SlugsVideoCamControl();

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

private:
    Ui::SlugsVideoCamControl *ui;
    bool dragging;
};

#endif // SLUGSVIDEOCAMCONTROL_H
