#include <QSettings>

#include "HUD2FormHorizon.h"

HUD2FormHorizon::HUD2FormHorizon(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2FormHorizon)
{
    ui->setupUi(this);
}

HUD2FormHorizon::~HUD2FormHorizon()
{
    delete ui;
}
