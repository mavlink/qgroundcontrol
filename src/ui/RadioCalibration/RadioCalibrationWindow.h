#ifndef RADIOCALIBRATIONWINDOW_H
#define RADIOCALIBRATIONWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>

#include "AirfoilServoCalibrator.h"
#include "SwitchCalibrator.h"
#include "CurveCalibrator.h"

class RadioCalibrationWindow : public QWidget
{
Q_OBJECT
public:
    explicit RadioCalibrationWindow(QWidget *parent = 0);

signals:

public slots:
    void setChannel(int ch, float raw, float normalized);

protected:
        AirfoilServoCalibrator *aileron;
        AirfoilServoCalibrator *elevator;
        AirfoilServoCalibrator *rudder;
        SwitchCalibrator *gyro;        
        CurveCalibrator *pitch;
        CurveCalibrator *throttle;

};

#endif // RADIOCALIBRATIONWINDOW_H
