#include "QGCUASFileViewMulti.h"
#include "ui_QGCUASFileViewMulti.h"
#include "UASInterface.h"
#include "MultiVehicleManager.h"
#include "QGCUASFileView.h"

QGCUASFileViewMulti::QGCUASFileViewMulti(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCUASFileViewMulti)
{
    ui->setupUi(this);
    setMinimumSize(600, 80);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &QGCUASFileViewMulti::_activeVehicleChanged);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded, this, &QGCUASFileViewMulti::_vehicleAdded);
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this, &QGCUASFileViewMulti::_vehicleRemoved);

    if (MultiVehicleManager::instance()->activeVehicle()) {
        _vehicleAdded(MultiVehicleManager::instance()->activeVehicle());
        _activeVehicleChanged(MultiVehicleManager::instance()->activeVehicle());
    }
}

void QGCUASFileViewMulti::_vehicleRemoved(Vehicle* vehicle)
{
    UAS* uas = vehicle->uas();
    Q_ASSERT(uas);
    
    QGCUASFileView* list = lists.value(uas, NULL);
    if (list)
    {
        delete list;
        lists.remove(uas);
    }
}

void QGCUASFileViewMulti::_vehicleAdded(Vehicle* vehicle)
{
    UAS* uas = vehicle->uas();
    
    if (!lists.contains(uas)) {
        QGCUASFileView* list = new QGCUASFileView(ui->stackedWidget, uas->getFileManager());
        lists.insert(uas, list);
        ui->stackedWidget->addWidget(list);
    }
}

void QGCUASFileViewMulti::_activeVehicleChanged(Vehicle* vehicle)
{
    if (vehicle) {
        QGCUASFileView* list = lists.value(vehicle->uas(), NULL);
        if (list) {
            ui->stackedWidget->setCurrentWidget(list);
        }
    }
}

QGCUASFileViewMulti::~QGCUASFileViewMulti()
{
    delete ui;
}

void QGCUASFileViewMulti::changeEvent(QEvent *e)
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
