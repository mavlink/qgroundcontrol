#include "QGCUnconnectedInfoWidget.h"
#include "LinkInterface.h"
#include "LinkManager.h"
#include "MainWindow.h"
#include "ui_QGCUnconnectedInfoWidget.h"

QGCUnconnectedInfoWidget::QGCUnconnectedInfoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCUnconnectedInfoWidget)
{
    ui->setupUi(this);
    //connect(ui->simulationButton, SIGNAL(clicked()), this, SLOT(simulate()));
    //connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(addLink()));
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
}

/**
 * @return Opens a "Connect new Link" popup
 */
void QGCUnconnectedInfoWidget::addLink()
{
    // TODO This doesn't make sense. If you want to connect, use the connect on the toolbar
    //MainWindow::instance()->addLink();
}
