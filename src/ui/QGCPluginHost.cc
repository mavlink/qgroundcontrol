#include "QGCPluginHost.h"
#include "ui_QGCPluginHost.h"

QGCPluginHost::QGCPluginHost(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCPluginHost)
{
    ui->setupUi(this);
}

QGCPluginHost::~QGCPluginHost()
{
    delete ui;
}
