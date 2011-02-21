#include "QGCMissionDoWidget.h"
#include "ui_QGCMissionDoWidget.h"

QGCMissionDoWidget::QGCMissionDoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCMissionDoWidget)
{
    ui->setupUi(this);
}

QGCMissionDoWidget::~QGCMissionDoWidget()
{
    delete ui;
}
