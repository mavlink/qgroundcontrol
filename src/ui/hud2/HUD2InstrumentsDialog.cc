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

    HUD2RibbonForm *left = new HUD2RibbonForm(speed->ribbon, this));
    ui->tabWidget->addTab(left, "test");
}

HUD2InstrumentsDialog::~HUD2InstrumentsDialog()
{
    delete ui;
}


