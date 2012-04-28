#include "QGCMissionConditionDelay.h"
#include "ui_QGCMissionConditionDelay.h"
#include "WaypointEditableView.h"

QGCMissionConditionDelay::QGCMissionConditionDelay(WaypointEditableView* WEV) :
    QWidget(WEV),
    ui(new Ui::QGCMissionConditionDelay)
{
    ui->setupUi(this);
    this->WEV = WEV;

    //Using UI to change WP:
    connect(this->ui->param1SpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam1(double)));

    //Reading WP to update UI:
    connect(WEV,SIGNAL(param1Broadcast(double)),this->ui->param1SpinBox,SLOT(setValue(double)));
}

QGCMissionConditionDelay::~QGCMissionConditionDelay()
{
    delete ui;
}
