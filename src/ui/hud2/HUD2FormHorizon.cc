#include <QSettings>

#include "HUD2FormHorizon.h"
#include "ui_HUD2FormHorizon.h"

HUD2FormHorizon::HUD2FormHorizon(HUD2IndicatorHorizon *horizon, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2FormHorizon),
    horizon(horizon)
{
    ui->setupUi(this);
    ui->checkBox->setChecked(horizon->getColoredBg());
    connect(ui->checkBox, SIGNAL(toggled(bool)), horizon, SLOT(setColoredBg(bool)));

    ui->bigScratchLenStep->setRange(1, 100);
    ui->bigScratchLenStep->setSingleStep(0.5);
    ui->bigScratchLenStep->setValue(horizon->getBigScratchLenStep());
    connect(ui->bigScratchLenStep, SIGNAL(valueChanged(double)),
            horizon, SLOT(setBigScratchLenStep(double)));

    ui->bigScratchValueStep->setRange(1, 10000);
    ui->bigScratchValueStep->setValue(horizon->getBigScratchValueStep());
    connect(ui->bigScratchValueStep, SIGNAL(valueChanged(int)),
            horizon, SLOT(setBigScratchValueStep(int)));

    ui->stepsBig->setRange(1, 10);
    ui->stepsBig->setSingleStep(1);
    ui->stepsBig->setValue(horizon->getStepsBig());
    connect(ui->stepsBig, SIGNAL(valueChanged(int)),
            horizon, SLOT(setStepsBig(int)));
}

HUD2FormHorizon::~HUD2FormHorizon()
{
    delete ui;
}
