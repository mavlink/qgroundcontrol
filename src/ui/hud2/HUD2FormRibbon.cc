#include <QSettings>

#include "HUD2FormRibbon.h"
#include "ui_HUD2FormRibbon.h"

HUD2FormRibbon::HUD2FormRibbon(HUD2Ribbon *ribbon, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HUD2FormRibbon)
{
    ui->setupUi(this);

    ui->comboBox->addItem("number");
    ui->comboBox->addItem("ribbon");
    ui->comboBox->addItem("both");

    ui->checkBoxNeedle->setChecked(ribbon->getOpacityNeedle());
    ui->checkBoxRibbon->setChecked(ribbon->getOpacityRibbon());
    ui->checkBoxEnable->setChecked(ribbon->getEnabled());
    connect(ui->checkBoxNeedle, SIGNAL(toggled(bool)), ribbon, SLOT(setOpacityNeedle(bool)));
    connect(ui->checkBoxRibbon, SIGNAL(toggled(bool)), ribbon, SLOT(setOpacityRibbon(bool)));
    connect(ui->checkBoxEnable, SIGNAL(toggled(bool)), ribbon, SLOT(setEnabled(bool)));

    ui->bigScratchLenStep->setRange(1, 100);
    ui->bigScratchLenStep->setSingleStep(0.5);
    ui->bigScratchLenStep->setValue(ribbon->getBigScratchLenStep());
    connect(ui->bigScratchLenStep, SIGNAL(valueChanged(double)),
            ribbon, SLOT(setBigScratchLenStep(double)));

    ui->bigScratchValueStep->setRange(1, 10000);
    ui->bigScratchValueStep->setValue(ribbon->getBigScratchValueStep());
    connect(ui->bigScratchValueStep, SIGNAL(valueChanged(int)),
            ribbon, SLOT(setBigScratchValueStep(int)));

    ui->stepsSmall->setRange(0, 10);
    ui->stepsSmall->setValue(ribbon->getStepsSmall());
    connect(ui->stepsSmall, SIGNAL(valueChanged(int)),
            ribbon, SLOT(setStepsSmall(int)));

    ui->stepsBig->setRange(1, 10);
    ui->stepsBig->setSingleStep(1);
    ui->stepsBig->setValue(ribbon->getStepsBig());
    connect(ui->stepsBig, SIGNAL(valueChanged(int)),
            ribbon, SLOT(setStepsBig(int)));
}

HUD2FormRibbon::~HUD2FormRibbon()
{
    delete ui;
}

void HUD2FormRibbon::on_comboBox_activated(int index){
    Q_UNUSED(index);
}

