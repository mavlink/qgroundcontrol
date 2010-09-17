#include "SwitchCalibrator.h"

SwitchCalibrator::SwitchCalibrator(QString titleString, QWidget *parent) :
    QWidget(parent)
{
    /* Add title label*/
    QLabel *title = new QLabel(titleString);
    QGridLayout *grid = new QGridLayout();
    grid->addWidget(title, 0, 0, 1, 3);

    /* Add current Pulse Width Display */
    QLabel *pulseWidthTitle = new QLabel(tr("Pulse Width (us)"));
    pulseWidth = new QLabel();
    QHBoxLayout *pulseLayout = new QHBoxLayout();
    pulseLayout->addWidget(pulseWidthTitle);
    pulseLayout->addWidget(pulseWidth);
    grid->addLayout(pulseLayout, 1, 0, 1, 3);

    this->setLayout(grid);
}

void SwitchCalibrator::channelChanged(float raw)
{
    pulseWidth->setText(QString::number(static_cast<double>(raw)));
}
