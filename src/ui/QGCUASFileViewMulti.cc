#include "QGCUASFileViewMulti.h"
#include "ui_QGCUASFileViewMulti.h"
#include "UASInterface.h"
#include "MultiVehicleManager.h"
#include "QGCUASFileView.h"
#include "QGCApplication.h"

QGCUASFileViewMulti::QGCUASFileViewMulti(const QString& title, QAction* action, QWidget *parent) :
    QGCDockWidget(title, action, parent),
    ui(new Ui::QGCUASFileViewMulti)
{
    ui->setupUi(this);
    setMinimumSize(600, 80);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &QGCUASFileViewMulti::_activeVehicleChanged);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &QGCUASFileViewMulti::_vehicleAdded);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved, this, &QGCUASFileViewMulti::_vehicleRemoved);

    if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        _vehicleAdded(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
        _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
    }
    
    loadSettings();
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
        QGCUASFileView* list = new QGCUASFileView(ui->stackedWidget, vehicle);
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
