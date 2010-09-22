#ifndef RADIOCALIBRATIONDATA_H
#define RADIOCALIBRATIONDATA_H

#include <QObject>
#include <QDebug>
#include <QVector>


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
    const float* operator[](int i) const;
    const QVector<float>& operator()(int i) const;

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
