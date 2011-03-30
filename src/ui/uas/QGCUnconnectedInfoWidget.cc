#include "QGCUnconnectedInfoWidget.h"
#include "LinkInterface.h"
#include "LinkManager.h"
#include "MAVLinkSimulationLink.h"
#include "MainWindow.h"
#include "ui_QGCUnconnectedInfoWidget.h"

QGCUnconnectedInfoWidget::QGCUnconnectedInfoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCUnconnectedInfoWidget)
{
    ui->setupUi(this);

    connect(ui->simulationButton, SIGNAL(clicked()), this, SLOT(simulate()));
    connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(addLink()));
}

QGCUnconnectedInfoWidget::~QGCUnconnectedInfoWidget()
{
    delete ui;
}

/**
 * @brief Starts the system simulation
 */
void QGCUnconnectedInfoWidget::simulate()
{
    // Try to get reference to MAVLinkSimulationlink
    QList<LinkInterface*> links = LinkManager::instance()->getLinks();
    foreach(LinkInterface* link, links) {
        MAVLinkSimulationLink* sim = dynamic_cast<MAVLinkSimulationLink*>(link);
        if (sim) {
            sim->connectLink();
        }
    }
}

/**
 * @return Opens a "Connect new Link" popup
 */
void QGCUnconnectedInfoWidget::addLink()
{
    MainWindow::instance()->addLink();
}
