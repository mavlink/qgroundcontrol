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

    ui->startButton->setText(tr("Connect"));

    QGCXPlaneLink* xplane = dynamic_cast<QGCXPlaneLink*>(link);

    if (xplane)
    {
        connect(ui->randomAttitudeButton, SIGNAL(clicked()), link, SLOT(setRandomAttitude()));
        connect(ui->randomPositionButton, SIGNAL(clicked()), link, SLOT(setRandomPosition()));
        connect(ui->airframeComboBox, SIGNAL(activated(QString)), link, SLOT(setAirframe(QString)));
    }
//    connect(ui->)
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
