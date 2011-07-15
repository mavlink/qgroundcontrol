#include "AirfoilServoCalibrator.h"

AirfoilServoCalibrator::AirfoilServoCalibrator(AirfoilType type, QWidget *parent) :
    AbstractCalibrator(parent),
    highPulseWidth(new QLabel()),
    centerPulseWidth(new QLabel()),
    lowPulseWidth(new QLabel())
{
    QGridLayout *grid = new QGridLayout(this);

    /* Add title */
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel* title;
    if (type == AILERON) {
        title = new QLabel(tr("Aileron"));
    } else if (type == ELEVATOR) {
        title = new QLabel(tr("Elevator"));
    } else if (type == RUDDER) {
        title = new QLabel(tr("Rudder"));
    } else {
        title = new QLabel(tr("Unknown"));
    }

    titleLayout->addWidget(title);
    grid->addLayout(titleLayout, 0, 0, 1, 3, Qt::AlignHCenter);

    /* Add current Pulse Width Display */
    QLabel *pulseWidthTitle = new QLabel(tr("Pulse Width (us)"));
    QHBoxLayout *pulseLayout = new QHBoxLayout();
    pulseLayout->addWidget(pulseWidthTitle);
    pulseLayout->addWidget(pulseWidth);
    grid->addLayout(pulseLayout, 1, 0, 1, 3);

    QLabel *highPulseString;
    QLabel *centerPulseString;
    QLabel *lowPulseString;
    if (type == AILERON) {
        highPulseString = new QLabel(tr("Bank Right"));
        centerPulseString = new QLabel(tr("Center"));
        lowPulseString = new QLabel(tr("Bank Left"));
    } else if (type == ELEVATOR) {
        highPulseString = new QLabel(tr("Nose Up"));
        centerPulseString = new QLabel(tr("Center"));
        lowPulseString = new QLabel(tr("Nose Down"));
    } else if (type == RUDDER) {
        highPulseString = new QLabel(tr("Nose Right"));
        centerPulseString = new QLabel(tr("Center"));
        lowPulseString = new QLabel(tr("Nose Left"));
    } else {
        highPulseString = new QLabel(tr("High"));
        centerPulseString = new QLabel(tr("Center"));
        lowPulseString = new QLabel(tr("Low"));
    }


    QPushButton *highButton = new QPushButton(tr("Set"));
    QPushButton *centerButton = new QPushButton(tr("Set"));
    QPushButton *lowButton = new QPushButton(tr("Set"));

    grid->addWidget(highPulseString, 2, 0);
    grid->addWidget(highPulseWidth, 2, 1);
    grid->addWidget(highButton, 2, 2);

    grid->addWidget(centerPulseString, 3, 0);
    grid->addWidget(centerPulseWidth, 3, 1);
    grid->addWidget(centerButton, 3, 2);

    grid->addWidget(lowPulseString, 4, 0);
    grid->addWidget(lowPulseWidth, 4, 1);
    grid->addWidget(lowButton, 4, 2);

    this->setLayout(grid);

    connect(highButton, SIGNAL(clicked()), this, SLOT(setHigh()));
    connect(centerButton, SIGNAL(clicked()), this, SLOT(setCenter()));
    connect(lowButton, SIGNAL(clicked()), this, SLOT(setLow()));
}

void AirfoilServoCalibrator::setHigh()
{
    highPulseWidth->setText(QString::number(logExtrema()));
    emit setpointChanged(2, logExtrema());
}

void AirfoilServoCalibrator::setCenter()
{
    centerPulseWidth->setText(QString::number(logAverage()));
    emit setpointChanged(1, logAverage());
}

void AirfoilServoCalibrator::setLow()
{
    lowPulseWidth->setText(QString::number(logExtrema()));
    emit setpointChanged(0, logExtrema());
}

void AirfoilServoCalibrator::set(const QVector<uint16_t> &data)
{
    if (data.size() == 3) {
        lowPulseWidth->setText(QString::number(data[0]));
        centerPulseWidth->setText(QString::number(data[1]));
        highPulseWidth->setText(QString::number(data[2]));
    }
}
