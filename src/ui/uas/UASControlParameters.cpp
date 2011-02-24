#include "src\ui\uas\UASControlParameters.h"
#include "ui_UASControlParameters.h"

#define CONTROL_MODE_LOCKED "MODE LOCKED"
#define CONTROL_MODE_MANUAL "MODE MANUAL"
#define CONTROL_MODE_GUIDED "MODE GUIDED"
#define CONTROL_MODE_AUTO   "MODE AUTO"
#define CONTROL_MODE_TEST1  "MODE TEST1"
#define CONTROL_MODE_TEST2  "MODE TEST2"
#define CONTROL_MODE_TEST3  "MODE TEST3"
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

    //this->mode = "MAV_MODE_UNKNOWN";
    //connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(activeUasSet(UASInterface*)));

    connect(ui->btGetCommands, SIGNAL(clicked()), this, SLOT(getCommands()));

    //QColor groupColor = QColor(231,72,28);
    //QString borderColor = "#FA4A4F";
    //groupColor = groupColor.darker(475);
}

UASControlParameters::~UASControlParameters()
{
    delete ui;
}

void UASControlParameters::changedMode(int mode)
{
    QString modeTemp;

    if (mode == CONTROL_MODE_LOCKED_INDEX)
    {
        modeTemp= CONTROL_MODE_LOCKED;
    }
    else if (mode == CONTROL_MODE_MANUAL_INDEX)
    {
        modeTemp= CONTROL_MODE_MANUAL;
    }
    else if (mode == CONTROL_MODE_GUIDED_INDEX)
    {
        modeTemp= CONTROL_MODE_GUIDED;
    }
    else if (mode == CONTROL_MODE_AUTO_INDEX)
    {
        modeTemp= CONTROL_MODE_AUTO;
    }
    else if (mode == CONTROL_MODE_TEST1_INDEX)
    {
        modeTemp= CONTROL_MODE_TEST1;
    }
    else if (mode == CONTROL_MODE_TEST2_INDEX)
    {
        modeTemp= CONTROL_MODE_TEST2;
    }
    else if (mode == CONTROL_MODE_TEST3_INDEX)
    {
        modeTemp= CONTROL_MODE_TEST3;
    }
    else if (mode == CONTROL_MODE_RC_TRAINING_INDEX)
    {
        modeTemp= CONTROL_MODE_RC_TRAINING;
    }

    if( static_cast<QString>(modeTemp) != this->mode)
    {
        ui->lbMode->setStyleSheet("background-color: rgb(255, 0, 0)");
    }
    else
    {
        ui->lbMode->setStyleSheet("");
    }
}

void UASControlParameters::activeUasSet(UASInterface *uas)
{
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(speedChanged(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));

    activeUAS= uas;
}

void UASControlParameters::updateGlobalPosition(UASInterface * a, double b, double c, double aa, quint64 ab)
{
    //ui->sbHeight->setValue(aa);
    this->altitude=aa;
}

void UASControlParameters::speedChanged(UASInterface* uas, double vx, double vy, double vz, quint64 time)
{
    this->speed = sqrt(pow(vx, 2.0) + pow(vy, 2.0) + pow(vz, 2.0));
    //ui->sbAirSpeed->setValue(speed);
}

void UASControlParameters::updateAttitude(UASInterface *uas, double roll, double pitch, double yaw, quint64 time)
{
    Q_UNUSED(uas);
    Q_UNUSED(time);
    //ui->sbTurnRate->setValue(roll);
    this->roll = roll;
}

void UASControlParameters::setCommands()
{}

void UASControlParameters::getCommands()
{
    ui->sbAirSpeed->setValue(this->speed);
    ui->sbHeight->setValue(this->altitude);
    ui->sbTurnRate->setValue(this->roll);
}

void UASControlParameters::updateMode(int uas,QString mode,QString description)
{
    Q_UNUSED(uas);
    Q_UNUSED(description);
    this->mode = mode;
    ui->lbMode->setText(this->mode);
}
