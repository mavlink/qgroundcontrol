#include "QGCMapToolbar.h"
#include "ui_QGCMapToolbar.h"

QGCMapToolbar::QGCMapToolbar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCMapToolbar)
{
    ui->setupUi(this);
}

QGCMapToolbar::~QGCMapToolbar()
{
    delete ui;
}
