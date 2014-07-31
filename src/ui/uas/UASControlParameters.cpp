#include "UASControlParameters.h"
#include "ui_UASControlParameters.h"

#define CONTROL_MODE_LOCKED "MODE LOCKED"
#define CONTROL_MODE_MANUAL "MODE MANUAL"

#define CONTROL_MODE_READY  "MODE TEST3"
#define CONTROL_MODE_RC_TRAINING  "RC SIMULATION"

#define CONTROL_MODE_LOCKED_INDEX 1
#define CONTROL_MODE_MANUAL_INDEX 2
#define CONTROL_MODE_GUIDED_INDEX 3
#define CONTROL_MODE_AUTO_INDEX   4
#define CONTROL_MODE_TEST1_INDEX  5
#define CONTROL_MODE_TEST2_INDEX  6
#define CONTROL_MODE_TEST3_INDEX  7
#define CONTROL_MODE_READY_INDEX  8
#define CONTROL_MODE_RC_TRAINING_INDEX  9

UASControlParameters::UASControlParameters(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UASControlParameters)
{
    ui->setupUi(this);

    ui->btSetCtrl->setStatusTip(tr("Set Passthrough"));

    connect(ui->btGetCommands, SIGNAL(clicked()), this, SLOT(getCommands()));

    connect(ui->btSetCtrl, SIGNAL(clicked()), this, SLOT(setPassthrough()));
}

UASControlParameters::~UASControlParameters()
{
    delete ui;
}

void UASControlParameters::changedMode(int mode)
{
    QString modeTemp;

    switch (mode) {
    case (uint8_t)MAV_MODE_PREFLIGHT:
        modeTemp = "LOCKED MODE";
        break;
    case (uint8_t)MAV_MODE_MANUAL_ARMED:
        modeTemp = "A/MANUAL MODE";
        break;
    case (uint8_t)MAV_MODE_MANUAL_DISARMED:
        modeTemp = "D/MANUAL MODE";
        break;
    default:
        modeTemp = "UNKNOWN MODE";
        break;
    }


    if(modeTemp != this->mode) {
        ui->lbMode->setStyleSheet("background-color: rgb(165, 42, 42)");
    } else {
        ui->lbMode->setStyleSheet("background-color: rgb(85, 107, 47)");
    }
}

void UASControlParameters::activeUasSet(UASInterface *uas)
{
    if(uas) {
        connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(velocityChanged_NED(UASInterface*,double,double,double,quint64)), this, SLOT(speedChanged(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        connect(uas, SIGNAL(thrustChanged(UASInterface*,double)), this, SLOT(thrustChanged(UASInterface*,double)) );

        activeUAS= uas;
    }
}

void UASControlParameters::updateGlobalPosition(UASInterface * a, double b, double c, double aa, quint64 ab)
{
    Q_UNUSED(a);
    Q_UNUSED(b);
    Q_UNUSED(c);
    Q_UNUSED(ab);
    this->altitude=aa;
}

void UASControlParameters::speedChanged(UASInterface* uas, double vx, double vy, double vz, quint64 time)
{
    Q_UNUSED(time);
    Q_UNUSED(uas);
    this->speed = sqrt(pow(vx, 2.0) + pow(vy, 2.0) + pow(vz, 2.0));
    //ui->sbAirSpeed->setValue(speed);
}

void UASControlParameters::updateAttitude(UASInterface *uas, double roll, double pitch, double yaw, quint64 time)
{
    Q_UNUSED(uas);
    Q_UNUSED(pitch);
    Q_UNUSED(yaw);
    Q_UNUSED(time);
    //ui->sbTurnRate->setValue(roll);
    this->roll = roll;
}

void UASControlParameters::setCommands()
{
}

void UASControlParameters::getCommands()
{
    ui->sbAirSpeed->setValue(this->speed);
    ui->sbHeight->setValue(this->altitude);
    ui->sbTurnRate->setValue(this->roll);
}

void UASControlParameters::setPassthrough()
{
}

void UASControlParameters::updateMode(int uas,QString mode,QString description)
{
    Q_UNUSED(uas);
    Q_UNUSED(description);
    this->mode = mode;
    ui->lbMode->setText(this->mode);

    ui->lbMode->setStyleSheet("background-color: rgb(85, 107, 47)");
}

void UASControlParameters::thrustChanged(UASInterface *uas, double throttle)
{
    Q_UNUSED(uas);
    this->throttle= throttle;
}
