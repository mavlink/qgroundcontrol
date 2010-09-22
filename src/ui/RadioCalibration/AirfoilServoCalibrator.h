#ifndef AIRFOILSERVOCALIBRATOR_H
#define AIRFOILSERVOCALIBRATOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include <QGridLayout>
#include <QHBoxLayout>

#include "AbstractCalibrator.h"

class AirfoilServoCalibrator : public AbstractCalibrator
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

    /** @param data must have exaclty 3 elemets.  they are assumed to be low center high */
    void set(const QVector<float>& data);

//signals:
//    void highSetpointChanged(float);
//    void centerSetpointChanged(float);
//    void lowSetpointChanged(float);

protected slots:
    void setHigh();
    void setCenter();
    void setLow();

protected:    
    QLabel *highPulseWidth;
    QLabel *centerPulseWidth;
    QLabel *lowPulseWidth;

//    float high;
//    float center;
//    float low;
};

#endif // AIRFOILSERVOCALIBRATOR_H
