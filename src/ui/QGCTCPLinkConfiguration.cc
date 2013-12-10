#include <QInputDialog>

#include "QGCTCPLinkConfiguration.h"
#include "ui_QGCTCPLinkConfiguration.h"

QGCTCPLinkConfiguration::QGCTCPLinkConfiguration(TCPLink* link, QWidget *parent) :
    QWidget(parent),
    link(link),
    ui(new Ui::QGCTCPLinkConfiguration)
{
    ui->setupUi(this);
    quint16 port = link->getPort();
    ui->portSpinBox->setValue(port);
    QString addr = link->getHostAddress().toString();
    ui->hostAddressLineEdit->setText(addr);
    connect(ui->portSpinBox, SIGNAL(valueChanged(int)), link, SLOT(setPort(int)));
    connect(ui->hostAddressLineEdit, SIGNAL(textChanged (const QString &)), link, SLOT(setAddress(const QString &)));
}

QGCTCPLinkConfiguration::~QGCTCPLinkConfiguration()
{
    delete ui;
}

void QGCTCPLinkConfiguration::changeEvent(QEvent *e)
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
