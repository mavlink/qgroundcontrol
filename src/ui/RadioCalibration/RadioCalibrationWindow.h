#ifndef RADIOCALIBRATIONWINDOW_H
#define RADIOCALIBRATIONWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QVector>

class RadioCalibrationWindow : public QWidget
{
Q_OBJECT
public:
    explicit RadioCalibrationWindow(QWidget *parent = 0);

signals:

public slots:

protected:
    class AirfoilServoCalibrator : public QWidget
    {
        Q_OBJECT
    public:
        explicit AirfoilServoCalibrator(QWidget *parent=0);

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

};

#endif // RADIOCALIBRATIONWINDOW_H
