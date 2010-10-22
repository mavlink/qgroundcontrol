#include "AbstractCalibrator.h"

AbstractCalibrator::AbstractCalibrator(QWidget *parent) :
    QWidget(parent),
    pulseWidth(new QLabel()),
    log(new QVector<float>())
{
}

AbstractCalibrator::~AbstractCalibrator()
{
    delete log;
}

float AbstractCalibrator::logAverage()
{
    float total = 0;
    for (int i=0; i<log->size(); ++i)
        total += log->value(i);
    return floor(total/log->size());
}

float AbstractCalibrator::logExtrema()
{
    float extrema = logAverage();
    if (logAverage() < 1500)
    {
        for (int i=0; i<log->size(); ++i)
        {
            if (log->value(i) < extrema)
                extrema = log->value(i);
        }
        extrema -= 5; // add 5us to prevent integer overflow
    }
    else
    {
        for (int i=0; i<log->size(); ++i)
        {
            if (log->value(i) > extrema)
                extrema = log->value(i);
        }
        extrema += 5; // subtact 5us to prevent integer overflow
    }

    return extrema;
}

void AbstractCalibrator::channelChanged(float raw)
{
    pulseWidth->setText(QString::number(static_cast<double>(raw)));
    if (log->size() == 5)
        log->pop_front();
    log->push_back(raw);
}
