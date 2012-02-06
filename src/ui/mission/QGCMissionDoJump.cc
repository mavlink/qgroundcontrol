#include "QGCMissionDoJump.h"
#include "ui_QGCMissionDoJump.h"
#include "WaypointEditableView.h"

QGCMissionDoJump::QGCMissionDoJump(WaypointEditableView* WEV) :
    QWidget(WEV),
    ui(new Ui::QGCMissionDoJump)
{
    ui->setupUi(this);
    this->WEV = WEV;

    //Using UI to change WP:
    connect(this->ui->param1SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam1(double)));
    connect(this->ui->param2SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam2(double)));

    //Reading WP to update UI:
    connect(WEV,SIGNAL(param1Broadcast(double)),this->ui->param1SpinBox,SLOT(setValue(double)));
    connect(WEV,SIGNAL(param2Broadcast(double)),this->ui->param2SpinBox,SLOT(setValue(double)));
}

QGCMissionDoJump::~QGCMissionDoJump()
{
    delete ui;
}
