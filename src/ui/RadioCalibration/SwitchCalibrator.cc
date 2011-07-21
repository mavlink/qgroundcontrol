#include "SwitchCalibrator.h"

SwitchCalibrator::SwitchCalibrator(QString titleString, QWidget *parent) :
    AbstractCalibrator(parent),
    defaultPulseWidth(new QLabel()),
    toggledPulseWidth(new QLabel())
{
    /* Add title label*/
    QLabel *title = new QLabel(titleString);
    QGridLayout *grid = new QGridLayout();
    grid->addWidget(title, 0, 0, 1, 3);

    /* Add current Pulse Width Display */
    QLabel *pulseWidthTitle = new QLabel(tr("Pulse Width (us)"));
    QHBoxLayout *pulseLayout = new QHBoxLayout();
    pulseLayout->addWidget(pulseWidthTitle);
    pulseLayout->addWidget(pulseWidth);
    grid->addLayout(pulseLayout, 1, 0, 1, 3);

    QLabel *defaultPulseString = new QLabel(tr("Default Position"));
    QPushButton *defaultButton = new QPushButton(tr("Set"));
    grid->addWidget(defaultPulseString, 2, 0);
    grid->addWidget(defaultPulseWidth, 2, 1);
    grid->addWidget(defaultButton, 2, 2);

    QLabel *toggledPulseString = new QLabel(tr("Toggled Position"));
    QPushButton *toggledButton = new QPushButton(tr("Set"));
    grid->addWidget(toggledPulseString, 3, 0);
    grid->addWidget(toggledPulseWidth, 3, 1);
    grid->addWidget(toggledButton, 3, 2);

    this->setLayout(grid);

    connect(defaultButton, SIGNAL(clicked()), this, SLOT(setDefault()));
    connect(toggledButton, SIGNAL(clicked()), this, SLOT(setToggled()));
}


void SwitchCalibrator::setDefault()
{
    defaultPulseWidth->setText(QString::number(logExtrema()));
    emit setpointChanged(0, logExtrema());
}

void SwitchCalibrator::setToggled()
{
    toggledPulseWidth->setText(QString::number(logExtrema()));
    emit setpointChanged(1, logExtrema());
}

void SwitchCalibrator::set(const QVector<uint16_t> &data)
{
    if (data.size() == 2) {
        defaultPulseWidth->setText(QString::number(data[0]));
        toggledPulseWidth->setText(QString::number(data[1]));
    }
}
