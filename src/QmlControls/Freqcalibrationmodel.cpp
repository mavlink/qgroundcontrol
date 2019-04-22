#include "Freqcalibrationmodel.h"

FreqCalibrationModel::FreqCalibrationModel(QObject *parent) : QObject(parent)
{

}
FreqCalibrationModel::FreqCalibrationModel(const QString &color, QObject *parent)
    : QObject(parent), c_color(color)
{
}

QString FreqCalibrationModel::color() const
{
    return c_color;
}

void FreqCalibrationModel::setColor(const QString &value)
{
    if (value != c_color) {
        c_color = value;
        emit colorChanged();
    }
}




CliTestModel::CliTestModel(QObject *parent) : QObject(parent)
{

}
CliTestModel::CliTestModel(const float &value, QObject *parent)
    : QObject(parent), x_value(value)
{
}

float CliTestModel::value() const
{
    return x_value;
}

void CliTestModel::setValue(const float &value)
{
    if (value != x_value) {
        x_value = value;
        emit valueChanged();
    }
}

