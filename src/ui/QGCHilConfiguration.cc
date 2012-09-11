#include "QGCHilConfiguration.h"
#include "ui_QGCHilConfiguration.h"

QGCHilConfiguration::QGCHilConfiguration(QGCHilLink* link, QWidget *parent) :
    QWidget(parent),
    link(link),
    ui(new Ui::QGCHilConfiguration)
{
    ui->setupUi(this);

    connect(ui->startButton, SIGNAL(clicked(bool)), this, SLOT(toggleSimulation(bool)));
    connect(ui->hostComboBox, SIGNAL(activated(QString)), link, SLOT(setRemoteHost(QString)));

    ui->startButton->setText(tr("Connect"));
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
