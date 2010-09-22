#ifndef SWITCHCALIBRATOR_H
#define SWITCHCALIBRATOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>

#include "AbstractCalibrator.h"

class SwitchCalibrator : public AbstractCalibrator
{
Q_OBJECT
public:
    explicit SwitchCalibrator(QString title=QString(), QWidget *parent = 0);

    void set(const QVector<float> &data);
//signals:
//    void defaultSetpointChanged(float);
//    void toggledSetpointChanged(float);

protected slots:
    void setDefault();
    void setToggled();

protected:   
    QLabel *defaultPulseWidth;
    QLabel *toggledPulseWidth;    

};

#endif // SWITCHCALIBRATOR_H
