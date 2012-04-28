#include "QGCMissionNavSweep.h"
#include "ui_QGCMissionNavSweep.h"
#include "WaypointEditableView.h"

QGCMissionNavSweep::QGCMissionNavSweep(WaypointEditableView* WEV) :
    QWidget(WEV),
    ui(new Ui::QGCMissionNavSweep)
{
    ui->setupUi(this);
    this->WEV = WEV;

    //Using UI to change WP:
    connect(this->ui->radSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam1(double)));
    //connect(this->ui->acceptanceSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam2(double)));
    connect(this->ui->posN1SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam3(double)));//NED
    connect(this->ui->posE1SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam4(double)));
    connect(this->ui->posN2SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam5(double)));
    connect(this->ui->posE2SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam6(double)));
    connect(this->ui->posDSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam7(double)));
    connect(this->ui->lat1SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam3(double)));//Global
    connect(this->ui->lon1SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam4(double)));
    connect(this->ui->lat2SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam5(double)));
    connect(this->ui->lon2SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam6(double)));
    connect(this->ui->altSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam7(double)));

    //Reading WP to update UI:
    connect(WEV,SIGNAL(frameBroadcast(MAV_FRAME)),this,SLOT(updateFrame(MAV_FRAME)));
    connect(WEV,SIGNAL(param1Broadcast(double)),this->ui->radSpinBox,SLOT(setValue(double)));
    //connect(WEV,SIGNAL(param2Broadcast(double)),this->ui->acceptanceSpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param3Broadcast(double)),this->ui->posN1SpinBox,SLOT(setValue(double)));//NED
    connect(WEV,SIGNAL(param4Broadcast(double)),this->ui->posE1SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param5Broadcast(double)),this->ui->posN2SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param6Broadcast(double)),this->ui->posE2SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param7Broadcast(double)),this->ui->posDSpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param3Broadcast(double)),this->ui->lat1SpinBox,SLOT(setValue(double)));//Global
    connect(WEV,SIGNAL(param4Broadcast(double)),this->ui->lon1SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param5Broadcast(double)),this->ui->lat2SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param6Broadcast(double)),this->ui->lon2SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param7Broadcast(double)),this->ui->altSpinBox,SLOT(setValue(double)));
}

void QGCMissionNavSweep::updateFrame(MAV_FRAME frame)
{
    switch(frame)
    {
    case MAV_FRAME_LOCAL_ENU:
    case MAV_FRAME_LOCAL_NED:
        this->ui->posN1SpinBox->show();
        this->ui->posE1SpinBox->show();
        this->ui->posN2SpinBox->show();
        this->ui->posE2SpinBox->show();
        this->ui->posDSpinBox->show();
        this->ui->lat1SpinBox->hide();
        this->ui->lon1SpinBox->hide();
        this->ui->lat2SpinBox->hide();
        this->ui->lon2SpinBox->hide();
        this->ui->altSpinBox->hide();
        break;
    case MAV_FRAME_GLOBAL:
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        this->ui->posN1SpinBox->hide();
        this->ui->posE1SpinBox->hide();
        this->ui->posN2SpinBox->hide();
        this->ui->posE2SpinBox->hide();
        this->ui->posDSpinBox->hide();
        this->ui->lat1SpinBox->show();
        this->ui->lon1SpinBox->show();
        this->ui->lat2SpinBox->show();
        this->ui->lon2SpinBox->show();
        this->ui->altSpinBox->show();
        break;
    default:
        break;
    }
}

QGCMissionNavSweep::~QGCMissionNavSweep()
{
    delete ui;
}
