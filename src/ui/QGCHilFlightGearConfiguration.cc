#include "QGCHilFlightGearConfiguration.h"
#include "ui_QGCHilFlightGearConfiguration.h"

#include "MainWindow.h"

QGCHilFlightGearConfiguration::QGCHilFlightGearConfiguration(UAS* mav,QWidget *parent) :
    QWidget(parent),
    mav(mav),
    ui(new Ui::QGCHilFlightGearConfiguration)
{
    ui->setupUi(this);

    QStringList items = QStringList();
    if (mav->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        items << "EasyStar";
        items << "Rascal110-JSBSim";
        items << "c172p";
        items << "YardStik";
        items << "Malolo1";
    }
    else if (mav->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        items << "arducopter";
    }
    else
    {
        items << "<aircraft>";
    }
    ui->aircraftComboBox->addItems(items);
}

QGCHilFlightGearConfiguration::~QGCHilFlightGearConfiguration()
{
    delete ui;
}

void QGCHilFlightGearConfiguration::on_startButton_clicked()
{
    //XXX check validity of inputs
    QString options = ui->optionsPlainTextEdit->toPlainText();
    options.append(" --aircraft=" + ui->aircraftComboBox->currentText());
    mav->enableHilFlightGear(true,  options);
}

void QGCHilFlightGearConfiguration::on_stopButton_clicked()
{
    mav->stopHil();
}
