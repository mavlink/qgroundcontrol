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

#include "AirfoilServoCalibrator.h"
#include "SwitchCalibrator.h"
#include "CurveCalibrator.h"

#include "UASManager.h"
#include "UASInterface.h"
#include "mavlink.h"

class RadioCalibrationWindow : public QWidget
{
Q_OBJECT

public:
    explicit RadioCalibrationWindow(QWidget *parent = 0);

signals:

public slots:
    void setChannel(int ch, float raw, float normalized);
    void loadFile() {this->radio->loadFile();}
    void saveFile() {this->radio->saveFile();}
    void send() {this->radio->send();}
    void receive() {this->radio->receive();}
    void setUASId(int id) {this->radio->setUASId(id);}


protected:
        AirfoilServoCalibrator *aileron;
        AirfoilServoCalibrator *elevator;
        AirfoilServoCalibrator *rudder;
        SwitchCalibrator *gyro;        
        CurveCalibrator *pitch;
        CurveCalibrator *throttle;

        class RadioCalibrationData
        {
        public:
            explicit RadioCalibrationData(RadioCalibrationWindow *parent=0);
            RadioCalibrationData(RadioCalibrationData&);            
            RadioCalibrationData(const QVector<float>& aileron,
                                 const QVector<float>& elevator,
                                 const QVector<float>& rudder,
                                 const QVector<float>& gyro,
                                 const QVector<float>& pitch,
                                 const QVector<float>& throttle,
                                 RadioCalibrationWindow *parent=0);

            enum RadioElement
            {
                AILERON=0,
                ELEVATOR,
                RUDDER,
                GYRO,
                PITCH,
                THROTTLE
            };

            void loadFile();
            void saveFile();
            void send();
            void receive();
            void setUASId(int id);

        protected:
            QVector<QVector<float> > *data;
            int uasID;

            void init(const QVector<float>& aileron,
                      const QVector<float>& elevator,
                      const QVector<float>& rudder,
                      const QVector<float>& gyro,
                      const QVector<float>& pitch,
                      const QVector<float>& throttle);

        private:
            RadioCalibrationWindow *parent;


        };

        RadioCalibrationData *radio;
};

#endif // RADIOCALIBRATIONWINDOW_H
