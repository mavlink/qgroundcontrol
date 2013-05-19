#include <QSettings>

#include "HUD2FormHorizon.h"

HUD2FormHorizon::HUD2FormHorizon(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2FormHorizon)
{
    ui->setupUi(this);
//    ui->checkBox->setChecked(parent->getColoredBg());
//    connect(ui->checkBox, SIGNAL(toggled(bool)), parent, SLOT(setColoredBg(bool)));

//    ui->bigScratchLenStep->setRange(1, 100);
//    ui->bigScratchLenStep->setSingleStep(0.5);
//    ui->bigScratchLenStep->setValue(horizon->getBigScratchLenStep());
//    connect(ui->bigScratchLenStep, SIGNAL(valueChanged(double)),
//            parent, SLOT(setBigScratchLenStep(double)));

//    ui->bigScratchValueStep->setRange(1, 10000);
//    ui->bigScratchValueStep->setValue(horizon->getBigScratchValueStep());
//    connect(ui->bigScratchValueStep, SIGNAL(valueChanged(int)),
//            parent, SLOT(setBigScratchValueStep(int)));

//    ui->stepsBig->setRange(1, 10);
//    ui->stepsBig->setSingleStep(1);
//    ui->stepsBig->setValue(horizon->getStepsBig());
//    connect(ui->stepsBig, SIGNAL(valueChanged(int)),
//            parent, SLOT(setStepsBig(int)));
}

HUD2FormHorizon::~HUD2FormHorizon()
{
    delete ui;
}
