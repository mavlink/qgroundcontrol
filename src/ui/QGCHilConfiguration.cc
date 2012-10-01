#include "QGCHilConfiguration.h"
#include "ui_QGCHilConfiguration.h"
#include "QGCXPlaneLink.h"

QGCHilConfiguration::QGCHilConfiguration(QGCHilLink* link, QWidget *parent) :
    QWidget(parent),
    link(link),
    ui(new Ui::QGCHilConfiguration)
{
    ui->setupUi(this);

    connect(ui->startButton, SIGNAL(clicked(bool)), this, SLOT(toggleSimulation(bool)));
    connect(ui->hostComboBox, SIGNAL(activated(QString)), link, SLOT(setRemoteHost(QString)));
    connect(link, SIGNAL(remoteChanged(QString)), ui->hostComboBox, SLOT(setEditText(QString)));
    connect(link, SIGNAL(statusMessage(QString)), this, SLOT(receiveStatusMessage(QString)));
    connect(link, SIGNAL(versionChanged(QString)), ui->simComboBox, SLOT(setEditText(QString)));
    connect(ui->simComboBox, SIGNAL(activated(QString)), link, SLOT(setVersion(QString)));
    ui->simComboBox->setEditText(link->getVersion());

    ui->startButton->setText(tr("Connect"));

    QGCXPlaneLink* xplane = dynamic_cast<QGCXPlaneLink*>(link);

    if (xplane)
    {
        connect(ui->randomAttitudeButton, SIGNAL(clicked()), link, SLOT(setRandomAttitude()));
        connect(ui->randomPositionButton, SIGNAL(clicked()), link, SLOT(setRandomPosition()));
        connect(ui->airframeComboBox, SIGNAL(activated(QString)), link, SLOT(setAirframe(QString)));
        ui->airframeComboBox->setCurrentIndex(link->getAirFrameIndex());
    }

    ui->hostComboBox->clear();
    ui->hostComboBox->addItem(link->getRemoteHost());
//    connect(ui->)
}

void QGCHilConfiguration::receiveStatusMessage(const QString& message)
{
    ui->statusLabel->setText(message);
}

void QGCHilConfiguration::toggleSimulation(bool connect)
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

QGCHilConfiguration::~QGCHilConfiguration()
{
    delete ui;
}
