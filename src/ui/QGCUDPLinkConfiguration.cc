#include <QInputDialog>

#include "QGCUDPLinkConfiguration.h"
#include "ui_QGCUDPLinkConfiguration.h"

QGCUDPLinkConfiguration::QGCUDPLinkConfiguration(UDPLink* link, QWidget *parent) :
    QWidget(parent),
    link(link),
    ui(new Ui::QGCUDPLinkConfiguration)
{
    ui->setupUi(this);
    ui->portSpinBox->setValue(link->getPort());
    connect(ui->portSpinBox, SIGNAL(valueChanged(int)), link, SLOT(setPort(int)));
    connect(ui->addIPButton, SIGNAL(clicked()), this, SLOT(addHost()));
}

QGCUDPLinkConfiguration::~QGCUDPLinkConfiguration()
{
    delete ui;
}

void QGCUDPLinkConfiguration::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void QGCUDPLinkConfiguration::addHost()
{
    bool ok;
    QString hostName = QInputDialog::getText(this, tr("Add a new IP address / hostname to MAVLink"),
                       tr("Host (hostname:port):"), QLineEdit::Normal,
                       "localhost:14555", &ok);
    if (ok && !hostName.isEmpty())
        link->addHost(hostName);
}
