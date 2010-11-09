#include "slugshilsim.h"
#include "ui_slugshilsim.h"

SlugsHilSim::SlugsHilSim(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsHilSim)
{
    ui->setupUi(this);
}

SlugsHilSim::~SlugsHilSim()
{
    delete ui;
}
