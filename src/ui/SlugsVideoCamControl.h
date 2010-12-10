#ifndef SLUGSVIDEOCAMCONTROL_H
#define SLUGSVIDEOCAMCONTROL_H

#include <QWidget>
#include <QMouseEvent>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include "SlugsPadCameraControl.h"
#include <QPushButton>



#define DELTA 1000

namespace Ui {
    class SlugsVideoCamControl;
}

class SlugsVideoCamControl : public QWidget

{
    Q_OBJECT

public:
    explicit SlugsVideoCamControl(QWidget *parent = 0);
    ~SlugsVideoCamControl();

public slots:
    void changeViewCamBorderAtMapStatus(bool status);
    void getDeltaPositionPad(double dir, double dist, QString dirText);


    void mousePadPressEvent(int x, int y);
    void mousePadReleaseEvent(int x, int y);
    void mousePadMoveEvent(int x, int y);

signals:
    void changeCamPosition(double dist, double dir, QString textDir);
    void viewCamBorderAtMap(bool status);

protected:
   void mousePressEvent(QMouseEvent* event);
   void mouseReleaseEvent(QMouseEvent* event);
   void mouseMoveEvent(QMouseEvent* event);




private:
    Ui::SlugsVideoCamControl *ui;

    SlugsPadCameraControl* padCamera;

};

#endif // SLUGSVIDEOCAMCONTROL_H
