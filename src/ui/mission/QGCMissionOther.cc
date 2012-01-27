#include "QGCMissionOther.h"
#include "ui_QGCMissionOther.h"

QGCMissionOther::QGCMissionOther(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCMissionOther)
{
    ui->setupUi(this);
}

QGCMissionOther::~QGCMissionOther()
{
    delete ui;
}
