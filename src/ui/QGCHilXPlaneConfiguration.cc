#include "QGCHilXPlaneConfiguration.h"
#include "ui_QGCHilXPlaneConfiguration.h"
#include "QGCXPlaneLink.h"
#include "QGCHilConfiguration.h"

QGCHilXPlaneConfiguration::QGCHilXPlaneConfiguration(QGCHilLink* link, QGCHilConfiguration *parent) :
    QWidget(parent),
    ui(new Ui::QGCHilXPlaneConfiguration)
{
    ui->setupUi(this);
    this->link = link;

    connect(ui->startButton, &QPushButton::clicked, this, &QGCHilXPlaneConfiguration::toggleSimulation);

    connect(ui->hostComboBox, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated),
            link, &QGCHilLink::setRemoteHost);

    connect(link, &QGCHilLink::remoteChanged, ui->hostComboBox, &QComboBox::setEditText);
    connect(link, &QGCHilLink::statusMessage, parent, &QGCHilConfiguration::receiveStatusMessage);

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
        connect(xplane, &QGCXPlaneLink::sensorHilChanged, ui->sensorHilCheckBox, &QCheckBox::setChecked);
        connect(ui->sensorHilCheckBox, &QCheckBox::clicked, xplane, &QGCXPlaneLink::enableSensorHIL);

        connect(link, static_cast<void (QGCHilLink::*)(int)>(&QGCHilLink::versionChanged),
                this, &QGCHilXPlaneConfiguration::setVersion);
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
