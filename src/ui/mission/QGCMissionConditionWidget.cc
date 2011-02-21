#include "QGCMissionConditionWidget.h"
#include "ui_QGCMissionConditionWidget.h"

QGCMissionConditionWidget::QGCMissionConditionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCMissionConditionWidget)
{
    ui->setupUi(this);
}

QGCMissionConditionWidget::~QGCMissionConditionWidget()
{
    delete ui;
}
