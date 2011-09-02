#include "UASControlParameters.h"
#include "ui_UASControlParameters.h"

#define CONTROL_MODE_LOCKED "MODE LOCKED"
#define CONTROL_MODE_MANUAL "MODE MANUAL"

#ifdef MAVLINK_ENABLED_SLUGS
#define CONTROL_MODE_GUIDED "MODE MID-L CMDS"
#define CONTROL_MODE_AUTO   "MODE WAYPOINT"
#define CONTROL_MODE_TEST1  "MODE PASST"
#define CONTROL_MODE_TEST2  "MODE SEL PT"
#else
#define CONTROL_MODE_GUIDED "MODE GUIDED"
#define CONTROL_MODE_AUTO   "MODE AUTO"
#define CONTROL_MODE_TEST1  "MODE TEST1"
#define CONTROL_MODE_TEST2  "MODE TEST2"
#endif

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
#ifdef MAVLINK_ENABLED_SLUGS
    case (uint8_t)MAV_MODE_AUTO:
        modeTemp = "WAYPOINT MODE";
        break;
    case (uint8_t)MAV_MODE_GUIDED:
        modeTemp = "MID-L CMDS MODE";
        break;

    case (uint8_t)MAV_MODE_TEST1:
        modeTemp = "PASST MODE";
        break;
    case (uint8_t)MAV_MODE_TEST2:
        modeTemp = "SEL PT MODE";
        break;
#endif
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
        connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(speedChanged(UASInterface*,double,double,double,quint64)));
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
#ifdef MAVLINK_ENABLED_SLUGS
    if(this->activeUAS) {
        UAS* myUas= static_cast<UAS*>(this->activeUAS);

        mavlink_message_t msg;

        tempCmds.uCommand = ui->sbAirSpeed->value();
        tempCmds.hCommand = ui->sbHeight->value();
        tempCmds.rCommand = ui->sbTurnRate->value();

        mavlink_msg_mid_lvl_cmds_encode(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, &this->tempCmds);
        myUas->sendMessage(msg);
    }
#endif
}

void UASControlParameters::getCommands()
{
    ui->sbAirSpeed->setValue(this->speed);
    ui->sbHeight->setValue(this->altitude);
    ui->sbTurnRate->setValue(this->roll);
}

void UASControlParameters::setPassthrough()
{
#ifdef MAVLINK_ENABLED_SLUGS
    if(this->activeUAS) {
        UAS* myUas= static_cast<UAS*>(this->activeUAS);

        mavlink_message_t msg;

        int8_t tmpBit=0;

        if(ui->cxdle_c->isChecked()) { //left elevator command
            tmpBit+=8;
        }
        if(ui->cxdr_c->isChecked()) { //rudder command
            tmpBit+=16;
        }

        if(ui->cxdla_c->isChecked()) { //left aileron command
            tmpBit+=64;
        }
        if(ui->cxdt_c->isChecked()) { //throttle command
            tmpBit+=128;
        }

        generic_16bit r;
        r.b[1] = 0;
        r.b[0] = tmpBit;//255;

        tempCtrl.target= this->activeUAS->getUASID();
        tempCtrl.bitfieldPt= (uint16_t)r.s;

        mavlink_msg_ctrl_srfc_pt_encode(MG::SYSTEM::ID, MG::SYSTEM::COMPID, &msg, &this->tempCtrl);
        myUas->sendMessage(msg);
        //qDebug()<<tempCtrl.bitfieldPt;
    }
#endif
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
