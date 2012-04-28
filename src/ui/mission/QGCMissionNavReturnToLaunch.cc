#include "QGCMissionNavReturnToLaunch.h"
#include "ui_QGCMissionNavReturnToLaunch.h"
#include "WaypointEditableView.h"

QGCMissionNavReturnToLaunch::QGCMissionNavReturnToLaunch(WaypointEditableView* WEV) :
    QWidget(WEV),
    ui(new Ui::QGCMissionNavReturnToLaunch)
{
    ui->setupUi(this);
    this->WEV = WEV;
}

QGCMissionNavReturnToLaunch::~QGCMissionNavReturnToLaunch()
{
    delete ui;
}
