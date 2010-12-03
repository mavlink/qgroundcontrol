#ifndef SLUGSPADCAMERACONTROL_H
#define SLUGSPADCAMERACONTROL_H

#include <QWidget>

namespace Ui {
    class SlugsPadCameraControl;
}

class SlugsPadCameraControl : public QWidget
{
    Q_OBJECT

public:
    explicit SlugsPadCameraControl(QWidget *parent = 0);
    ~SlugsPadCameraControl();

private:
    Ui::SlugsPadCameraControl *ui;
};

#endif // SLUGSPADCAMERACONTROL_H
