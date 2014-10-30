#include "QGCConfigView.h"
#include "ui_QGCConfigView.h"
#include "UASManager.h"
#include "QGCPX4VehicleConfig.h"
#include "MainWindow.h"

QGCConfigView::QGCConfigView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCConfigView),
    config(NULL),
    mav(NULL)
{
    ui->setupUi(this);

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(activeUASChanged(UASInterface*)));

    ui->gridLayout->removeWidget(ui->waitingLabel);
    ui->waitingLabel->setVisible(false);
    delete ui->waitingLabel;
    ui->waitingLabel = NULL;

    config = new QGCPX4VehicleConfig();
    ui->gridLayout->addWidget(config);
}

QGCConfigView::~QGCConfigView()
{
    delete ui;
}

void QGCConfigView::activeUASChanged(UASInterface* uas)
{
    if (mav == uas)
        return;

    int type = -1;
    if (mav)
        type = mav->getAutopilotType();

    mav = uas;
    if (uas && type != uas->getAutopilotType()) {

        if (ui->waitingLabel) {
            ui->gridLayout->removeWidget(ui->waitingLabel);
            ui->waitingLabel->setVisible(false);
        }

        //remove all child widgets since they could contain stale data
        //for example, when we switch from one PX4 UAS to another UAS
        foreach (QObject* obj, ui->gridLayout->children()) {
            QWidget* w = dynamic_cast<QWidget*>(obj);
            if (w) {
                if (obj != ui->waitingLabel) {
                    ui->gridLayout->removeWidget(w);
                    delete obj;
                }
            }
        }

        QGCPX4VehicleConfig* px4config = qobject_cast<QGCPX4VehicleConfig*>(config);
        if (!px4config) {
            if (config)
                delete config;
            config = new QGCPX4VehicleConfig();
            ui->gridLayout->addWidget(config);
        }
    }
    else {
        if (ui->waitingLabel) {
            //restore waiting label if we no longer have a connection
            ui->gridLayout->addWidget(ui->waitingLabel);
            ui->waitingLabel->setVisible(true);
        }
    }

}
