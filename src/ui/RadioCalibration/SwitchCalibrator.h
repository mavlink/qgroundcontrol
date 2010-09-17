#ifndef SWITCHCALIBRATOR_H
#define SWITCHCALIBRATOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>

class SwitchCalibrator : public QWidget
{
Q_OBJECT
public:
    explicit SwitchCalibrator(QString title=QString(), QWidget *parent = 0);

signals:
    void defaultSetpointChanged(float);
    void toggledSetpointChanged(float);

public slots:
    void channelChanged(float raw);
protected:
    QLabel *pulseWidth;
    QPushButton *defaultButton;
    QPushButton *toggledButton;

    float defaultPos;
    float toggled;

    QVector<float> log;

};

#endif // SWITCHCALIBRATOR_H
