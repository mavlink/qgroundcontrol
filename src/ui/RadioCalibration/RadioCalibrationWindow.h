#ifndef RADIOCALIBRATIONWINDOW_H
#define RADIOCALIBRATIONWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QPointer>
#include <QFileDialog>
#include <QFile>
#include <QtXml>
#include <QTextStream>

#include "AirfoilServoCalibrator.h"
#include "SwitchCalibrator.h"
#include "CurveCalibrator.h"

#include "mavlink.h"
#include "mavlink_types.h"
#include "UAS.h"
#include "UASManager.h"
#include "RadioCalibrationData.h"


class RadioCalibrationWindow : public QWidget
{
Q_OBJECT

public:
    explicit RadioCalibrationWindow(QWidget *parent = 0);

public slots:
    void setChannel(int ch, float raw, float normalized);
    void loadFile();
    void saveFile();
    void send();
    void request();
    void receive(const QPointer<RadioCalibrationData>& radio);
    void setUASId(int id) {this->uasId = id;}


protected:
    AirfoilServoCalibrator *aileron;
    AirfoilServoCalibrator *elevator;
    AirfoilServoCalibrator *rudder;
    SwitchCalibrator *gyro;
    CurveCalibrator *pitch;
    CurveCalibrator *throttle;
    int uasId;
    QPointer<RadioCalibrationData> radio;
    QSignalMapper mapper;

    void parseSetpoint(const QDomElement& setpoint, const QPointer<RadioCalibrationData>& radio);
};

#endif // RADIOCALIBRATIONWINDOW_H
