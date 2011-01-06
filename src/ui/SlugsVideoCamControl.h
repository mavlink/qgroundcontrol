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
    /**
         * @brief status = true: emit signal to draw a border cam over the map
    */
    void changeViewCamBorderAtMapStatus(bool status);
    /**
         * @brief show the values of mousepad on ui (labels) and emit a changeCamPosition(signal)
         *        with values:
         *       bearing and distance from mouse over the pad
         *          dirText: direction of mouse movement in text format (up, right,right up,right down,
         *                   left, left up, left down, down)
    */
    void getDeltaPositionPad(double bearing, double distance, QString dirText);

//    /**
//         * @brief
//    */
//    void mousePadPressEvent(int x, int y);
//    void mousePadReleaseEvent(int x, int y);
//    void mousePadMoveEvent(int x, int y);

signals:
    /**
         * @brief emit values from mousepad:
         *       bearing and distance from mouse over the pad
         *          dirText: direction of mouse movement in text format (up, right,right up,right down,
         *                   left, left up, left down, down)
    */
    void changeCamPosition(double distance, double bearing, QString textDir);
    /**
         * @brief emit signal to draw a border cam over the map if status is true
    */
    void viewCamBorderAtMap(bool status);

protected:
//   void mousePressEvent(QMouseEvent* event);
//   void mouseReleaseEvent(QMouseEvent* event);
//   void mouseMoveEvent(QMouseEvent* event);




private:
    Ui::SlugsVideoCamControl *ui;

    SlugsPadCameraControl* padCamera;

};

#endif // SLUGSVIDEOCAMCONTROL_H
