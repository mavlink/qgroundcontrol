#include "QGCConfigView.h"
#include "ui_QGCConfigView.h"
#include "UASManager.h"
#include "QGCPX4VehicleConfig.h"
#include "QGCVehicleConfig.h"
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

    // The config screens are required for firmware uploading
    if (MainWindow::instance()->getCustomMode() == MainWindow::CUSTOM_MODE_PX4) {

        ui->gridLayout->removeWidget(ui->waitingLabel);
        ui->waitingLabel->setVisible(false);
        delete ui->waitingLabel;

        config = new QGCPX4VehicleConfig();
        ui->gridLayout->addWidget(config);

    } else {
        //don't show a configuration widget if no vehicle is connected
        //show a placeholder informational widget instead
    }

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
    if (NULL != uas && type != uas->getAutopilotType()) {

        ui->gridLayout->removeWidget(ui->waitingLabel);
        ui->waitingLabel->setVisible(false);

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

        int autopilotType = mav->getAutopilotType();
        switch (autopilotType) {
        case MAV_AUTOPILOT_PX4:
            {
                QGCPX4VehicleConfig* px4config = qobject_cast<QGCPX4VehicleConfig*>(config);
                if (!px4config) {
                    if (config)
                        delete config;
                    config = new QGCPX4VehicleConfig();
                    ui->gridLayout->addWidget(config);
                }
            }
            break;
        default:
            {
                QGCVehicleConfig* generalconfig = qobject_cast<QGCVehicleConfig*>(config);
                if (!generalconfig) {
                    if (config)
                        delete config;
                    config = new QGCVehicleConfig();
                    ui->gridLayout->addWidget(config);
                }
            }
            break;
        }
    }
    else {
        //restore waiting label if we no longer have a connection
        ui->gridLayout->addWidget(ui->waitingLabel);
        ui->waitingLabel->setVisible(true);
    }

}
