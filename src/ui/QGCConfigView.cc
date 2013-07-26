#include "QGCConfigView.h"
#include "ui_QGCConfigView.h"
#include "UASManager.h"
#include "QGCPX4VehicleConfig.h"
#include "QGCVehicleConfig.h"
#include "QGCPX4VehicleConfig.h"

QGCConfigView::QGCConfigView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCConfigView),
    currUAS(NULL)
{
    ui->setupUi(this);

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(activeUASChanged(UASInterface*)));

    if (ui->waitingLabel) {
        ui->gridLayout->removeWidget(ui->waitingLabel);
        delete ui->waitingLabel;
        ui->waitingLabel = NULL;
    }
    ui->gridLayout->addWidget(new QGCPX4VehicleConfig());
}

QGCConfigView::~QGCConfigView()
{
    delete ui;
}

void QGCConfigView::activeUASChanged(UASInterface* uas)
{
    if (currUAS == uas)
        return;

    if (ui->waitingLabel) {
        ui->gridLayout->removeWidget(ui->waitingLabel);
        delete ui->waitingLabel;
        ui->waitingLabel = NULL;
    }

    if (currUAS && currUAS->getAutopilotType() != uas->getAutopilotType()) {
        foreach (QObject* obj, ui->gridLayout->children()) {
            QWidget* w = dynamic_cast<QWidget*>(obj);
            if (w) {
                ui->gridLayout->removeWidget(w);
                delete obj;
            }
        }
    }

    switch (uas->getAutopilotType()) {
    case MAV_AUTOPILOT_PX4:
        ui->gridLayout->addWidget(new QGCPX4VehicleConfig());
    default:
        ui->gridLayout->addWidget(new QGCVehicleConfig());
    }
}
