#include "RadioCalibrationData.h"

RadioCalibrationData::RadioCalibrationData()
{
    data = new QVector<QVector<uint16_t> >(6);
    (*data).insert(AILERON, QVector<uint16_t>(3));
    (*data).insert(ELEVATOR, QVector<uint16_t>(3));
    (*data).insert(RUDDER, QVector<uint16_t>(3));
    (*data).insert(GYRO, QVector<uint16_t>(2));
    (*data).insert(PITCH, QVector<uint16_t>(5));
    (*data).insert(THROTTLE, QVector<uint16_t>(5));
}

RadioCalibrationData::RadioCalibrationData(const QVector<uint16_t> &aileron,
        const QVector<uint16_t> &elevator,
        const QVector<uint16_t> &rudder,
        const QVector<uint16_t> &gyro,
        const QVector<uint16_t> &pitch,
        const QVector<uint16_t> &throttle)
{
    data = new QVector<QVector<uint16_t> >();
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
    data = new QVector<QVector<uint16_t> >(*other.data);
}

RadioCalibrationData::~RadioCalibrationData()
{
    delete data;
}

const uint16_t* RadioCalibrationData::operator [](int i) const
{
    if (i < data->size()) {
        return (*data)[i].constData();
    }

    return NULL;
}

const QVector<uint16_t>& RadioCalibrationData::operator ()(const int i) const throw(std::out_of_range)
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
