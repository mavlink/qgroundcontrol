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
        items << QStringLiteral("EasyStar");
        items << QStringLiteral("Rascal110-JSBSim");
        items << QStringLiteral("c172p");
        items << QStringLiteral("YardStik");
        items << QStringLiteral("Malolo1");
    }
    else if (_vehicle->vehicleType() == MAV_TYPE_QUADROTOR)
    {
        items << QStringLiteral("arducopter");
    }
    else
    {
        items << QStringLiteral("<aircraft>");
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
