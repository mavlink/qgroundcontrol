#include "RadioCalibrationWindow.h"

RadioCalibrationWindow::RadioCalibrationWindow(QWidget *parent) :
        QWidget(parent, Qt::Window)
{
    QGridLayout *grid = new QGridLayout();

    aileron = new AirfoilServoCalibrator(AirfoilServoCalibrator::AILERON);
    grid->addWidget(aileron, 0, 0, 1, 1, Qt::AlignTop);

    elevator = new AirfoilServoCalibrator(AirfoilServoCalibrator::ELEVATOR);
    grid->addWidget(elevator, 0, 1, 1, 1, Qt::AlignTop);

    rudder = new AirfoilServoCalibrator(AirfoilServoCalibrator::RUDDER);
    grid->addWidget(rudder, 0, 2, 1, 1, Qt::AlignTop);

    gyro = new SwitchCalibrator(tr("Gyro Mode/Gain"));
    grid->addWidget(gyro, 0, 3, 1, 1, Qt::AlignTop);


    pitch = new CurveCalibrator(tr("Collective Pitch"));
    grid->addWidget(pitch, 1, 0, 1, 2);

    throttle = new CurveCalibrator(tr("Throttle"));
    grid->addWidget(throttle, 1, 2, 1, 2);

    this->setLayout(grid);
}

void RadioCalibrationWindow::setChannel(int ch, float raw, float normalized)
{
    /** this expects a particular channel to function mapping
       \todo allow run-time channel mapping
       */
    switch (ch)
    {
    case 0:
        aileron->channelChanged(raw);
        break;
    case 1:
        elevator->channelChanged(raw);
        break;
    case 2:
        throttle->channelChanged(raw);
        break;
    case 3:
        rudder->channelChanged(raw);
        break;
    case 4:
        gyro->channelChanged(raw);
        break;
    case 5:
        pitch->channelChanged(raw);
        break;


    }
}
