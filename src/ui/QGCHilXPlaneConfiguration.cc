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
        connect(ui->randomAttitudeButton, SIGNAL(clicked()), link, SLOT(setRandomAttitude()));
        connect(ui->randomPositionButton, SIGNAL(clicked()), link, SLOT(setRandomPosition()));
        connect(ui->airframeComboBox, SIGNAL(activated(QString)), link, SLOT(selectAirframe(QString)));
        ui->airframeComboBox->setCurrentIndex(link->getAirFrameIndex());
    }

    ui->hostComboBox->clear();
    ui->hostComboBox->addItem(link->getRemoteHost());


}

void QGCHilXPlaneConfiguration::toggleSimulation(bool connect)
{
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
