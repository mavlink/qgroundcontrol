#include "QGCMissionNavLand.h"
#include "ui_QGCMissionNavLand.h"
#include "WaypointEditableView.h"

QGCMissionNavLand::QGCMissionNavLand(WaypointEditableView* WEV) :
    QWidget(WEV),
    ui(new Ui::QGCMissionNavLand)
{
    ui->setupUi(this);
    this->WEV = WEV;

    //Using UI to change WP:
    //connect(this->ui->holdTimeSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam1(double)));
    //connect(this->ui->acceptanceSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam2(double)));
    //connect(this->ui->param3SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam3(double)));
    connect(this->ui->yawSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam4(double)));
    connect(this->ui->posNSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam5(double)));//NED
    connect(this->ui->posESpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam6(double)));
    connect(this->ui->posDSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam7(double)));
    connect(this->ui->latSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam5(double)));//Global
    connect(this->ui->lonSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam6(double)));
    connect(this->ui->altSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam7(double)));

    //Reading WP to update UI:
    connect(WEV,SIGNAL(frameBroadcast(MAV_FRAME)),this,SLOT(updateFrame(MAV_FRAME)));
    //connect(WEV,SIGNAL(param1Broadcast(double)),this->ui->holdTimeSpinBox,SLOT(setValue(double)));
    //connect(WEV,SIGNAL(param2Broadcast(double)),this->ui->acceptanceSpinBox,SLOT(setValue(double)));
    //connect(WEV,SIGNAL(param3Broadcast(double)),this->ui->param3SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param4Broadcast(double)),this->ui->yawSpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param5Broadcast(double)),this->ui->posNSpinBox,SLOT(setValue(double)));//NED
    connect(WEV,SIGNAL(param6Broadcast(double)),this->ui->posESpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param7Broadcast(double)),this->ui->posDSpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param5Broadcast(double)),this->ui->latSpinBox,SLOT(setValue(double)));//Global
    connect(WEV,SIGNAL(param6Broadcast(double)),this->ui->lonSpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param7Broadcast(double)),this->ui->altSpinBox,SLOT(setValue(double)));
}

void QGCMissionNavLand::updateFrame(MAV_FRAME frame)
{
    switch(frame)
    {
    case MAV_FRAME_LOCAL_ENU:
    case MAV_FRAME_LOCAL_NED:
        this->ui->posNSpinBox->show();
        this->ui->posESpinBox->show();
        this->ui->posDSpinBox->show();
        this->ui->latSpinBox->hide();
        this->ui->lonSpinBox->hide();
        this->ui->altSpinBox->hide();
        break;
    case MAV_FRAME_GLOBAL:
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        this->ui->posNSpinBox->hide();
        this->ui->posESpinBox->hide();
        this->ui->posDSpinBox->hide();
        this->ui->latSpinBox->show();
        this->ui->lonSpinBox->show();
        this->ui->altSpinBox->show();
        break;
    default:
        break;
    }
}

QGCMissionNavLand::~QGCMissionNavLand()
{
    delete ui;
}
