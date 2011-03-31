#include "RadioCalibrationData.h"

RadioCalibrationData::RadioCalibrationData()
{
    data = new QVector<QVector<float> >(6);
    (*data).insert(AILERON, QVector<float>(3));
    (*data).insert(ELEVATOR, QVector<float>(3));
    (*data).insert(RUDDER, QVector<float>(3));
    (*data).insert(GYRO, QVector<float>(2));
    (*data).insert(PITCH, QVector<float>(5));
    (*data).insert(THROTTLE, QVector<float>(5));
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

RadioCalibrationData::~RadioCalibrationData()
{
    delete data;
}

const float* RadioCalibrationData::operator [](int i) const
{
    if (i < data->size()) {
        return (*data)[i].constData();
    }

    return NULL;
}

const QVector<float>& RadioCalibrationData::operator ()(const int i) const throw(std::out_of_range)
{
    if ((i < data->size()) && (i >=0)) {
        return (*data)[i];
    }

    throw std::out_of_range("Invalid channel index");
}

QString RadioCalibrationData::toString(RadioElement element) const
{
    QString s;
    foreach (float f, (*data)[element]) {
        s += QString::number(f) + ", ";
    }
    return s.mid(0, s.length()-2);
}
