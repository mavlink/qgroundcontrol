#include "QGCHilXPlaneConfiguration.h"
#include "ui_QGCHilXPlaneConfiguration.h"
#include "QGCXPlaneLink.h"

QGCHilXPlaneConfiguration::QGCHilXPlaneConfiguration(QGCHilLink* link, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCHilXPlaneConfiguration)
{
    ui->setupUi(this);
    this->link = link;

    connect(ui->startButton, SIGNAL(clicked(bool)), this, SLOT(toggleSimulation(bool)));
    connect(ui->hostComboBox, SIGNAL(activated(QString)), link, SLOT(setRemoteHost(QString)));
    connect(link, SIGNAL(remoteChanged(QString)), ui->hostComboBox, SLOT(setEditText(QString)));
    connect(link, SIGNAL(statusMessage(QString)), parent, SLOT(receiveStatusMessage(QString)));

//    connect(mav->getHILSimulation(), SIGNAL(statusMessage(QString)), this, SLOT(receiveStatusMessage(QString)));
//    connect(ui->simComboBox, SIGNAL(activated(QString)), mav->getHILSimulation(), SLOT(setVersion(QString)));

    ui->startButton->setText(tr("Connect"));

    QGCXPlaneLink* xplane = dynamic_cast<QGCXPlaneLink*>(link);

    if (xplane)
    {
//        connect(ui->randomAttitudeButton, SIGNAL(clicked()), link, SLOT(setRandomAttitude()));
//        connect(ui->randomPositionButton, SIGNAL(clicked()), link, SLOT(setRandomPosition()));

        //ui->airframeComboBox->setCurrentIndex(link->getAirFrameIndex());
        //connect(ui->airframeComboBox, SIGNAL(activated(QString)), link, SLOT(selectAirframe(QString)));
        // XXX not implemented yet
        //ui->airframeComboBox->hide();
        ui->sensorHilCheckBox->setChecked(xplane->sensorHilEnabled());
        connect(xplane, SIGNAL(sensorHilChanged(bool)), ui->sensorHilCheckBox, SLOT(setChecked(bool)));
        connect(ui->sensorHilCheckBox, SIGNAL(clicked(bool)), xplane, SLOT(enableSensorHIL(bool)));

        connect(link, SIGNAL(versionChanged(int)), this, SLOT(setVersion(int)));
    }

    ui->hostComboBox->clear();
    ui->hostComboBox->addItem(link->getRemoteHost());


}

void QGCHilXPlaneConfiguration::setVersion(int version)
{
    Q_UNUSED(version);
}

void QGCHilXPlaneConfiguration::toggleSimulation(bool connect)
{
    if (!link) {
        return;
    }

    Q_UNUSED(connect);
    if (!link->isConnected())
    {
        link->setRemoteHost(ui->hostComboBox->currentText());
        link->connectSimulation();
        ui->startButton->setText(tr("Disconnect"));
    }
    else
    {
        link->disconnectSimulation();
        ui->startButton->setText(tr("Connect"));
    }
}

QGCHilXPlaneConfiguration::~QGCHilXPlaneConfiguration()
{
    delete ui;
}
