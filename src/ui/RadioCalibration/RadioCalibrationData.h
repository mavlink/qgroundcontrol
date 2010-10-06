#ifndef RADIOCALIBRATIONDATA_H
#define RADIOCALIBRATIONDATA_H

#include <QObject>
#include <QDebug>
#include <QVector>
#include <QString>


class RadioCalibrationData : public QObject
{
Q_OBJECT

public:
    explicit RadioCalibrationData();
    RadioCalibrationData(const RadioCalibrationData&);
    RadioCalibrationData(const QVector<float>& aileron,
                         const QVector<float>& elevator,
                         const QVector<float>& rudder,
                         const QVector<float>& gyro,
                         const QVector<float>& pitch,
                         const QVector<float>& throttle);
    ~RadioCalibrationData();

    enum RadioElement
    {
        AILERON=0,
        ELEVATOR,
        RUDDER,
        GYRO,
        PITCH,
        THROTTLE
    };

    const float* operator[](int i) const;
    const QVector<float>& operator()(int i) const;
    void set(int element, int index, float value) {(*data)[element][index] = value;}

public slots:
    void setAileron(int index, float value) {set(AILERON, index, value);}
    void setElevator(int index, float value) {set(ELEVATOR, index, value);}
    void setRudder(int index, float value) {set(RUDDER, index, value);}
    void setGyro(int index, float value) {set(GYRO, index, value);}
    void setPitch(int index, float value) {set(PITCH, index, value);}
    void setThrottle(int index, float value) {set(THROTTLE, index, value);}

public:
    /// Creates a comman seperated list of the values for a particular element
    QString toString(const RadioElement element) const;

protected:
    QVector<QVector<float> > *data;


    void init(const QVector<float>& aileron,
              const QVector<float>& elevator,
              const QVector<float>& rudder,
              const QVector<float>& gyro,
              const QVector<float>& pitch,
              const QVector<float>& throttle);

};

#endif // RADIOCALIBRATIONDATA_H
