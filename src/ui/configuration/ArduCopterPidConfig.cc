#include "ArduCopterPidConfig.h"
#include "ui_ArduCopterPidConfig.h"

ArduCopterPidConfig::ArduCopterPidConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ArduCopterPidConfig)
{
    ui->setupUi(this);
}

ArduCopterPidConfig::~ArduCopterPidConfig()
{
    delete ui;
}
