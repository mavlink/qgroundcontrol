#include "AbstractCalibrator.h"

AbstractCalibrator::AbstractCalibrator(QWidget *parent) :
    QWidget(parent),
    pulseWidth(new QLabel()),
    log(new QVector<uint16_t>())
{
}

AbstractCalibrator::~AbstractCalibrator()
{
    delete log;
}

uint16_t AbstractCalibrator::logAverage()
{
	// Short-circuit here if the log is empty otherwise we get a div-by-0 error.
	if (log->empty())
	{
		return 0;
	}

    uint16_t total = 0;
    for (int i=0; i<log->size(); ++i)
	{
        total += log->value(i);
	}
    return total/log->size();
}

uint16_t AbstractCalibrator::logExtrema()
{
    uint16_t extrema = logAverage();
    if (logAverage() < 1500)
	{
        for (int i=0; i<log->size(); ++i)
		{
            if (log->value(i) < extrema)
			{
                extrema = log->value(i); 
			}
        }
        extrema -= 5; // add 5us to prevent integer overflow
    }
	else
	{
        for (int i=0; i<log->size(); ++i)
		{
            if (log->value(i) > extrema)
			{
                extrema = log->value(i);
			}
        }
        extrema += 5; // subtact 5us to prevent integer overflow
    }

    return extrema;
}

void AbstractCalibrator::channelChanged(uint16_t raw)
{
    pulseWidth->setText(QString::number(raw));
    if (log->size() == 5)
	{
        log->pop_front();
	}
    log->push_back(raw);
}
