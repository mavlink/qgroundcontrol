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

    //don't show a configuration widget if no vehicle is connected
    //show a placeholder informational widget instead

}

QGCConfigView::~QGCConfigView()
{
    delete ui;
}

void QGCConfigView::activeUASChanged(UASInterface* uas)
{
    if (currUAS == uas)
        return;

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

    if (NULL != uas) {
        ui->gridLayout->removeWidget(ui->waitingLabel);
        ui->waitingLabel->setVisible(false);

        switch (uas->getAutopilotType()) {
        case MAV_AUTOPILOT_PX4:
            ui->gridLayout->addWidget(new QGCPX4VehicleConfig());
            break;
        default:
            ui->gridLayout->addWidget(new QGCVehicleConfig());
            break;
        }
    }
    else {
        //restore waiting label if we no longer have a connection
        ui->gridLayout->addWidget(ui->waitingLabel);
        ui->waitingLabel->setVisible(true);
    }

}
