#include "QGCHilJSBSimConfiguration.h"
#include "ui_QGCHilJSBSimConfiguration.h"

#include "MainWindow.h"
#include "UAS.h"

QGCHilJSBSimConfiguration::QGCHilJSBSimConfiguration(Vehicle* vehicle, QWidget *parent)
    : QWidget(parent)
    , _vehicle(vehicle)
    , ui(new Ui::QGCHilJSBSimConfiguration)
{
    ui->setupUi(this);

    QStringList items = QStringList();
    if (_vehicle->vehicleType() == MAV_TYPE_FIXED_WING)
    {
        items << "EasyStar";
        items << "Rascal110-JSBSim";
        items << "c172p";
        items << "YardStik";
        items << "Malolo1";
    }
    else if (_vehicle->vehicleType() == MAV_TYPE_QUADROTOR)
    {
        items << "arducopter";
    }
    else
    {
        items << "<aircraft>";
    }
    ui->aircraftComboBox->addItems(items);
}

QGCHilJSBSimConfiguration::~QGCHilJSBSimConfiguration()
{
    delete ui;
}

void QGCHilJSBSimConfiguration::on_startButton_clicked()
{
    //XXX check validity of inputs
    QString options = ui->optionsPlainTextEdit->toPlainText();
    options.append(" --script=" + ui->aircraftComboBox->currentText());
    _vehicle->uas()->enableHilJSBSim(true,  options);
}

void QGCHilJSBSimConfiguration::on_stopButton_clicked()
{
    _vehicle->uas()->stopHil();
}
