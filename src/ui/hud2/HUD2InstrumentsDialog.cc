#include "HUD2InstrumentsDialog.h"
#include "ui_HUD2InstrumentsDialog.h"

HUD2InstrumentsDialog::HUD2InstrumentsDialog(HUD2IndicatorHorizon *horizon,
                                             HUD2IndicatorRoll *roll,
                                             HUD2Ribbon *speed,
                                             HUD2Ribbon *climb,
                                             HUD2Ribbon *compass,
                                             HUD2IndicatorFps *fps,
                                             QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HUD2InstrumentsDialog)
{
    ui->setupUi(this);

    HUD2RibbonForm *speed_form = new HUD2RibbonForm(speed, this);
    HUD2RibbonForm *climb_form = new HUD2RibbonForm(climb, this);
    HUD2RibbonForm *compass_form = new HUD2RibbonForm(compass, this);
    HUD2HorizonForm *horizon_form = new HUD2HorizonForm(horizon, this);
    HUD2FpsForm *fps_form = new HUD2FpsForm(fps, this);

    ui->tabWidget->addTab(speed_form, "speed");
    ui->tabWidget->addTab(climb_form, "climb");
    ui->tabWidget->addTab(compass_form, "compass");
    ui->tabWidget->addTab(fps_form, "fps");
    ui->tabWidget->addTab(horizon_form, "horizon");
}

HUD2InstrumentsDialog::~HUD2InstrumentsDialog()
{
    delete ui;
}
