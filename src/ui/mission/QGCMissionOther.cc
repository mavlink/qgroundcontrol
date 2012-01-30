#include "QGCMissionOther.h"
#include "ui_QGCMissionOther.h"
#include "WaypointEditableView.h"

QGCMissionOther::QGCMissionOther(WaypointEditableView* WEV) :
    QWidget(WEV),
    ui(new Ui::QGCMissionOther)
{
    ui->setupUi(this);
    this->WEV = WEV;
    //Using UI to change WP:
    connect(this->ui->commandSpinBox,SIGNAL(valueChanged(int)),WEV,SLOT(changedCommand(int)));
    connect(this->ui->param1SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam1(double)));
    connect(this->ui->param2SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam2(double)));
    connect(this->ui->param3SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam3(double)));
    connect(this->ui->param4SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam4(double)));
    connect(this->ui->param5SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam5(double)));
    connect(this->ui->param6SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam6(double)));
    connect(this->ui->param7SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam7(double)));


    //Reading WP to update UI:
    connect(WEV,SIGNAL(commandBroadcast(int)),this->ui->commandSpinBox,SLOT(setValue(int)));
    connect(WEV,SIGNAL(param1Broadcast(double)),this->ui->param1SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param2Broadcast(double)),this->ui->param2SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param3Broadcast(double)),this->ui->param3SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param4Broadcast(double)),this->ui->param4SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param5Broadcast(double)),this->ui->param5SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param6Broadcast(double)),this->ui->param6SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param7Broadcast(double)),this->ui->param7SpinBox,SLOT(setValue(double)));
}

QGCMissionOther::~QGCMissionOther()
{
    delete ui;
}
