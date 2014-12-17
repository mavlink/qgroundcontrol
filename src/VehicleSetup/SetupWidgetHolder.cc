#include "SetupWidgetHolder.h"
#include "ui_SetupWidgetHolder.h"

SetupWidgetHolder::SetupWidgetHolder(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetupWidgetHolder)
{
    ui->setupUi(this);
}

SetupWidgetHolder::~SetupWidgetHolder()
{
    delete ui;
}
