#ifndef AIRFOILSERVOCALIBRATOR_H
#define AIRFOILSERVOCALIBRATOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>

class AirfoilServoCalibrator : public QWidget
{
Q_OBJECT
public:
    enum AirfoilType
    {
        AILERON,
        ELEVATOR,
        RUDDER
    };

    explicit AirfoilServoCalibrator(AirfoilType type = AILERON, QWidget *parent = 0);



signals:
    void highSetpointChanged(float);
    void centerSetpointChanged(float);
    void lowSetpointChanged(float);

public slots:
    void channelChanged(float raw);
protected:
    QLabel *pulseWidth;
    QPushButton *highButton;
    QPushButton *centerButton;
    QPushButton *lowButton;

    float high;
    float center;
    float low;

    QVector<float> log;
};

#endif // AIRFOILSERVOCALIBRATOR_H
