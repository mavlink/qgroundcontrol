#include "QGCLogDownloader.h"
#include "ui_QGCLogDownloader.h"

QGCLogDownloader::QGCLogDownloader(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCLogDownloader)
{
    ui->setupUi(this);
}

QGCLogDownloader::~QGCLogDownloader()
{
    delete ui;
}
