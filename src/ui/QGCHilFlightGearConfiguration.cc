#include "QGCHilFlightGearConfiguration.h"
#include "ui_QGCHilFlightGearConfiguration.h"

#include "MainWindow.h"

QGCHilFlightGearConfiguration::QGCHilFlightGearConfiguration(UAS* mav,QWidget *parent) :
    QWidget(parent),
    mav(mav),
    ui(new Ui::QGCHilFlightGearConfiguration)
{
    ui->setupUi(this);
}

QGCHilFlightGearConfiguration::~QGCHilFlightGearConfiguration()
{
    delete ui;
}

void QGCHilFlightGearConfiguration::on_startButton_clicked()
{
    mav->enableHilFlightGear(true, ui->optionsPlainTextEdit->toPlainText());
}
