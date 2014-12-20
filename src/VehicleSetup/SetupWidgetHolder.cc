#include "SetupWidgetHolder.h"
#include "ui_SetupWidgetHolder.h"

SetupWidgetHolder::SetupWidgetHolder(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SetupWidgetHolder)
{
    ui->setupUi(this);
}

SetupWidgetHolder::~SetupWidgetHolder()
{
    delete ui;
}

void SetupWidgetHolder::setInnerWidget(QWidget* widget)
{
    ui->setupWidgetLayout->addWidget(widget);
}
