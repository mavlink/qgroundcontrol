#include "AirfoilServoCalibrator.h"

AirfoilServoCalibrator::AirfoilServoCalibrator(AirfoilType type, QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *grid = new QGridLayout(this);

    /* Add title */
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *title;
    if (type == AILERON)
    {
        title = new QLabel(tr("Aileron"));
    }
    else if (type == ELEVATOR)
    {
        title = new QLabel(tr("Elevator"));
    }
    else if (type == RUDDER)
    {
        title = new QLabel(tr("Rudder"));
    }

    titleLayout->addWidget(title);
    grid->addLayout(titleLayout, 0, 0, 1, 3, Qt::AlignHCenter);

    /* Add current Pulse Width Display */
    QLabel *pulseWidthTitle = new QLabel(tr("Pulse Width (us)"));
    pulseWidth = new QLabel();
    QHBoxLayout *pulseLayout = new QHBoxLayout();
    pulseLayout->addWidget(pulseWidthTitle);
    pulseLayout->addWidget(pulseWidth);
    grid->addLayout(pulseLayout, 1, 0, 1, 3);

    this->setLayout(grid);
}

void AirfoilServoCalibrator::channelChanged(float raw)
{
    pulseWidth->setText(QString::number(static_cast<double>(raw)));
}
