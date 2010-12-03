#include "SlugsPadCameraControl.h"
#include "ui_SlugsPadCameraControl.h"

SlugsPadCameraControl::SlugsPadCameraControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsPadCameraControl)
{
    ui->setupUi(this);
}

SlugsPadCameraControl::~SlugsPadCameraControl()
{
    delete ui;
}
