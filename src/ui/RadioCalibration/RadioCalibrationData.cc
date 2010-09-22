#include "RadioCalibrationData.h"

RadioCalibrationData::RadioCalibrationData()
{
    data = new QVector<QVector<float> >();
}

RadioCalibrationData::RadioCalibrationData(const QVector<float> &aileron,
                                                                   const QVector<float> &elevator,
                                                                   const QVector<float> &rudder,
                                                                   const QVector<float> &gyro,
                                                                   const QVector<float> &pitch,
                                                                   const QVector<float> &throttle)
{
    data = new QVector<QVector<float> >();
    (*data) << aileron
            << elevator
            << rudder
            << gyro
            << pitch
            << throttle;
}

RadioCalibrationData::RadioCalibrationData(const RadioCalibrationData &other)
    :QObject()
{
    data = new QVector<QVector<float> >(*other.data);
}

const float* RadioCalibrationData::operator [](int i) const
{
    if (i < data->size())
    {
        return (*data)[i].constData();
    }

    return NULL;
}

const QVector<float>& RadioCalibrationData::operator ()(int i) const
{
    if (i < data->size())
    {
        return (*data)[i];
    }

    // This is not good.  If it is ever used after being returned it will cause a crash
    return QVector<float>();
}
