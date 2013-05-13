#include "HUD2InstrumentsDialog.h"
#include "ui_HUD2InstrumentsDialog.h"

HUD2InstrumentsDialog::HUD2InstrumentsDialog(HUD2IndicatorHorizon *horizon,
                                             HUD2IndicatorRoll *roll,
                                             HUD2IndicatorSpeed *speed,
                                             HUD2IndicatorClimb *climb,
                                             HUD2IndicatorCompass *compass,
                                             HUD2IndicatorFps *fps,
                                             QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HUD2InstrumentsDialog)
{
    ui->setupUi(this);

    HUD2RibbonForm *speed_form = new HUD2RibbonForm(speed, this);
    HUD2RibbonForm *climb_form = new HUD2RibbonForm(climb, this);
    HUD2RibbonForm *compass_form = new HUD2RibbonForm(compass, this);

    ui->tabWidget->addTab(speed_form,   "speed");
    ui->tabWidget->addTab(climb_form,   "climb");
    ui->tabWidget->addTab(compass_form, "compass");
}

HUD2InstrumentsDialog::~HUD2InstrumentsDialog()
{
    delete ui;
}


