#include <QProgressDialog>
#include <QDebug>
#include <QTimer>

#include "QGCPX4AirframeConfig.h"
#include "ui_QGCPX4AirframeConfig.h"

#include "UASManager.h"
#include "LinkManager.h"
#include "UAS.h"
#include "QGC.h"
#include "QGCMessageBox.h"

QGCPX4AirframeConfig::QGCPX4AirframeConfig(QWidget *parent) :
    QWidget(parent),
    mav(NULL),
    progress(NULL),
    pendingParams(0),
    configState(CONFIG_STATE_ABORT),
    selectedId(-1),
    ui(new Ui::QGCPX4AirframeConfig)
{
    ui->setupUi(this);

    // Fill the lists here manually in accordance with the list from:
    // https://github.com/PX4/Firmware/blob/master/ROMFS/px4fmu_common/init.d/rcS

    ui->simComboBox->addItem(tr("Plane (HilStar, X-Plane)"), 1000);
    ui->simComboBox->addItem(tr("Plane (Rascal, FlightGear)"), 1004);
    ui->simComboBox->addItem(tr("Quad X HIL"), 1001);
    ui->simComboBox->addItem(tr("Quad + HIL"), 1003);

    connect(ui->simPushButton, SIGNAL(clicked()), this, SLOT(simSelected()));
    connect(ui->simComboBox, SIGNAL(activated(int)), this, SLOT(simSelected(int)));
    ui->simPushButton->setEnabled(ui->simComboBox->count() > 0);

    ui->planeComboBox->addItem(tr("Multiplex Easystar 1/2"), 2100);
    ui->planeComboBox->addItem(tr("Hobbyking Bixler 1/2"), 2101);
    ui->planeComboBox->addItem(tr("3DR Skywalker"), 2102);
    ui->planeComboBox->addItem(tr("Skyhunter (1800 mm)"), 2103);

    connect(ui->planePushButton, SIGNAL(clicked()), this, SLOT(planeSelected()));
    connect(ui->planeComboBox, SIGNAL(activated(int)), this, SLOT(planeSelected(int)));
    ui->planePushButton->setEnabled(ui->planeComboBox->count() > 0);

    ui->flyingWingComboBox->addItem(tr("Z-84 Wing Wing (845 mm)"), 3033);
    ui->flyingWingComboBox->addItem(tr("TBS Caipirinha (850 mm)"), 3100);
    ui->flyingWingComboBox->addItem(tr("Bormatec Camflyer Q (800 mm)"), 3030);
    ui->flyingWingComboBox->addItem(tr("FX-61 Phantom FPV (1550 mm)"), 3031);
    ui->flyingWingComboBox->addItem(tr("FX-79 Buffalo (2000 mm)"), 3034);
    ui->flyingWingComboBox->addItem(tr("Skywalker X5 (1180 mm)"), 3032);
    ui->flyingWingComboBox->addItem(tr("Viper v2 (3000 mm)"), 3035);

    connect(ui->flyingWingPushButton, SIGNAL(clicked()), this, SLOT(flyingWingSelected()));
    connect(ui->flyingWingComboBox, SIGNAL(activated(int)), this, SLOT(flyingWingSelected(int)));

    ui->quadXComboBox->addItem(tr("DJI F330 8\" Quad"), 4010);
    ui->quadXComboBox->addItem(tr("DJI F450 10\" Quad"), 4011);
    ui->quadXComboBox->addItem(tr("X frame Quad UAVCAN"), 4012);
    ui->quadXComboBox->addItem(tr("AR.Drone Frame Quad"), 4008);
//    ui->quadXComboBox->addItem(tr("DJI F330 with MK BLCTRL"), 4017);
//    ui->quadXComboBox->addItem(tr("Mikrokopter X frame"), 4019);

    connect(ui->quadXPushButton, SIGNAL(clicked()), this, SLOT(quadXSelected()));
    connect(ui->quadXComboBox, SIGNAL(activated(int)), this, SLOT(quadXSelected(int)));
    ui->quadXPushButton->setEnabled(ui->quadXComboBox->count() > 0);

    ui->quadPlusComboBox->addItem(tr("Generic 10\" Quad +"), 5001);
//    ui->quadXComboBox->addItem(tr("Mikrokopter + frame"), 5020);

    connect(ui->quadPlusPushButton, SIGNAL(clicked()), this, SLOT(quadPlusSelected()));
    connect(ui->quadPlusComboBox, SIGNAL(activated(int)), this, SLOT(quadPlusSelected(int)));
    ui->quadPlusPushButton->setEnabled(ui->quadPlusComboBox->count() > 0);

    ui->hexaXComboBox->addItem(tr("Standard 10\" Hexa X"), 6001);
    ui->hexaXComboBox->addItem(tr("Coaxial 10\" Hexa X"), 11001);

    connect(ui->hexaXPushButton, SIGNAL(clicked()), this, SLOT(hexaXSelected()));
    connect(ui->hexaXComboBox, SIGNAL(activated(int)), this, SLOT(hexaXSelected(int)));
    ui->hexaXPushButton->setEnabled(ui->hexaXComboBox->count() > 0);

    ui->hexaPlusComboBox->addItem(tr("Standard 10\" Hexa"), 7001);

    connect(ui->hexaPlusPushButton, SIGNAL(clicked()), this, SLOT(hexaPlusSelected()));
    connect(ui->hexaPlusComboBox, SIGNAL(activated(int)), this, SLOT(hexaPlusSelected(int)));
    ui->hexaPlusPushButton->setEnabled(ui->hexaPlusComboBox->count() > 0);

    ui->octoXComboBox->addItem(tr("Standard 10\" Octo"), 8001);
    ui->octoXComboBox->addItem(tr("Coaxial 10\" Octo"), 12001);

    connect(ui->octoXPushButton, SIGNAL(clicked()), this, SLOT(octoXSelected()));
    connect(ui->octoXComboBox, SIGNAL(activated(int)), this, SLOT(octoXSelected(int)));
    ui->octoXPushButton->setEnabled(ui->octoXComboBox->count() > 0);

    ui->octoPlusComboBox->addItem(tr("Standard 10\" Octo"), 9001);

    connect(ui->octoPlusPushButton, SIGNAL(clicked()), this, SLOT(octoPlusSelected()));
    connect(ui->octoPlusComboBox, SIGNAL(activated(int)), this, SLOT(octoPlusSelected(int)));
    ui->octoPlusPushButton->setEnabled(ui->octoPlusComboBox->count() > 0);

    ui->hComboBox->addItem(tr("3DR Iris"), 10016);
    ui->hComboBox->addItem(tr("TBS Discovery"), 10015);
    ui->hComboBox->addItem(tr("SteadiDrone QU4D"), 10017);

    connect(ui->hPushButton, SIGNAL(clicked()), this, SLOT(hSelected()));
    connect(ui->hComboBox, SIGNAL(activated(int)), this, SLOT(hSelected(int)));
    ui->hPushButton->setEnabled(ui->hComboBox->count() > 0);

    connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(applyAndReboot()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    uncheckAll();
    
    setActiveUAS(UASManager::instance()->getActiveUAS());
}

void QGCPX4AirframeConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);

    if (parameterName.contains("SYS_AUTOSTART"))
    {
        int index = value.toInt();
        if (index > 0) {
            setAirframeID(index);
            ui->statusLabel->setText(tr("Onboard start script ID: #%1").arg(index));
        } else {
            uncheckAll();
            ui->statusLabel->setText(tr("System not configured for autostart."));
        }
    }
}

void QGCPX4AirframeConfig::setActiveUAS(UASInterface* uas)
{
    if (mav)
    {
        disconnect(mav, SIGNAL(parameterChanged(int,int,QString,QVariant)), this, SLOT(parameterChanged(int,int,QString,QVariant)));
        mav = NULL;
    }

    if (!uas)
        return;

    mav = uas;
    paramMgr = mav->getParamManager();

    connect(mav, SIGNAL(parameterChanged(int,int,QString,QVariant)), this, SLOT(parameterChanged(int,int,QString,QVariant)));
    
    // If the parameters are ready, we aren't going to get paramterChanged signals. So fake them in order to make the UI work.
    if (uas->getParamManager()->parametersReady()) {
        QVariant value;
        static const char* param = "SYS_AUTOSTART";

        QGCUASParamManagerInterface* paramMgr = uas->getParamManager();
        
        QList<int> compIds = paramMgr->getComponentForParam(param);
        Q_ASSERT(compIds.count() == 1);
        paramMgr->getParameterValue(compIds[0], param, value);
        parameterChanged(uas->getUASID(), compIds[0], param, value);
    }
}

void QGCPX4AirframeConfig::uncheckAll()
{
    ui->planePushButton->setChecked(false);
    ui->flyingWingPushButton->setChecked(false);
    ui->quadXPushButton->setChecked(false);
    ui->quadPlusPushButton->setChecked(false);
    ui->hexaXPushButton->setChecked(false);
    ui->hexaPlusPushButton->setChecked(false);
    ui->octoXPushButton->setChecked(false);
    ui->octoPlusPushButton->setChecked(false);
    ui->hPushButton->setChecked(false);
}

void QGCPX4AirframeConfig::setAirframeID(int id)
{

    qDebug() << "setAirframeID" << id;
    ui->statusLabel->setText(tr("Start script ID: #%1").arg(id));

    selectedId = id;

    // XXX too much boilerplate code here - this widget is really just
    // a quick hack to get started
    uncheckAll();

    if (id >= 1000 && id < 2000)
    {
        ui->simPushButton->setChecked(true);
        ui->simComboBox->setCurrentIndex(ui->simComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected simulation (ID: #%1)").arg(selectedId));
    }
    else if (id >= 2000 && id < 3000)
    {
        ui->planePushButton->setChecked(true);
        ui->planeComboBox->setCurrentIndex(ui->planeComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected plane (ID: #%1)").arg(selectedId));
    }
    else if (id >= 3000 && id < 4000)
    {
        ui->flyingWingPushButton->setChecked(true);
        ui->flyingWingComboBox->setCurrentIndex(ui->flyingWingComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected flying wing (ID: #%1)").arg(selectedId));
    }
    else if (id >= 4000 && id < 5000) {
        ui->quadXPushButton->setChecked(true);
        ui->quadXComboBox->setCurrentIndex(ui->quadXComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected quadrotor in X config (ID: #%1)").arg(selectedId));
    }
    else if (id >= 5000 && id < 6000) {
        ui->quadPlusPushButton->setChecked(true);
        ui->quadPlusComboBox->setCurrentIndex(ui->quadPlusComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected quadrotor in + config (ID: #%1)").arg(selectedId));
    }
    else if (id >= 6000 && id < 7000) {
        ui->hexaXPushButton->setChecked(true);
        ui->hexaXComboBox->setCurrentIndex(ui->hexaXComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected hexarotor in X config (ID: #%1)").arg(selectedId));
    }
    else if (id >= 7000 && id < 8000) {
        ui->hexaPlusPushButton->setChecked(true);
        ui->hexaPlusComboBox->setCurrentIndex(ui->hexaPlusComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected hexarotor in + config (ID: #%1)").arg(selectedId));
    }
    else if (id >= 8000 && id < 9000) {
        ui->octoXPushButton->setChecked(true);
        ui->octoXComboBox->setCurrentIndex(ui->octoXComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected octorotor in X config (ID: #%1)").arg(selectedId));
    }
    else if (id >= 9000 && id < 10000) {
        ui->octoPlusPushButton->setChecked(true);
        ui->octoPlusComboBox->setCurrentIndex(ui->octoPlusComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected octorotor in + config (ID: #%1)").arg(selectedId));
    }
    else if (id >= 10000 && id < 11000)
    {
        ui->hPushButton->setChecked(true);
        ui->hComboBox->setCurrentIndex(ui->hComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected H frame multirotor (ID: #%1)").arg(selectedId));
    }


}

void QGCPX4AirframeConfig::applyAndReboot()
{
    // Guard against the case of an edit where we didn't receive all params yet
    if (selectedId <= 0)
    {
        QGCMessageBox::warning(tr("No airframe selected"),
                               tr("Please select an airframe first."));
        return;
    }

    if (!mav)
        return;

    if (paramMgr->countOnboardParams() == 0 &&
            paramMgr->countPendingParams() == 0)
    {
        paramMgr->requestParameterList();
        QGC::SLEEP::msleep(300);
    }

    QList<int> components = paramMgr->getComponentForParam("SYS_AUTOSTART");

    // Guard against the case of an edit where we didn't receive all params yet
    if (paramMgr->countPendingParams() > 0 || components.count() == 0)
    {
        QGCMessageBox::information(tr("Parameter sync with UAS not yet complete"),
                                   tr("Please wait a few moments and retry"));
        return;
    }

    // Guard against multiple components responding - this will never show in practice
    if (components.count() != 1) {
        QGCMessageBox::warning(tr("Invalid system setup detected"),
                               tr("None or more than one component advertised to provide the main system configuration option. This is an invalid system setup - please check your autopilot."));
        return;
    }

    // This is really evil: 'fake' a thread by
    // periodic work queue calls and clock
    // through a small state machine
    // ugh.. if we just had time to do this properly.

    // To the reader who can't program and wants to whine:
    // this is not beautiful, but technically completely
    // sound. If you want to fix it, you'd be welcome
    // to rebase the link, param manager and UI classes
    // on a proper threading framework - which I'd love to do
    // if I just had more time..

    configState = CONFIG_STATE_SEND;
    QTimer::singleShot(200, this, SLOT(checkConfigState()));
    setEnabled(false);
}

void QGCPX4AirframeConfig::checkConfigState()
{

    if (configState == CONFIG_STATE_SEND)
    {
        QList<int> components = paramMgr->getComponentForParam("SYS_AUTOSTART");
        qDebug() << "Setting comp" << components.first() << "SYS_AUTOSTART" << (qint32)selectedId;

        paramMgr->setPendingParam(components.first(),"SYS_AUTOSTART", (qint32)selectedId);

        //need to set autoconfig in order for PX4 to pick up the selected airframe params
        if (ui->defaultGainsCheckBox->checkState() == Qt::Checked)
                setAutoConfig(true);

        // Send pending params and then write them to persistent storage when done
        paramMgr->sendPendingParameters(true);

        configState = CONFIG_STATE_WAIT_PENDING;
        pendingParams = 0;
        QTimer::singleShot(2000, this, SLOT(checkConfigState()));
        return;
    }

    if (configState == CONFIG_STATE_WAIT_PENDING) {
        // Guard against the case of an edit where we didn't receive all params yet
        if (paramMgr->countPendingParams() > 0)
        {
            if (pendingParams == 0) {

                pendingParams = paramMgr->countPendingParams();

                if (progress)
                    delete progress;

                progress = new QProgressDialog("Writing parameters", "Abort Send", 0, pendingParams, this);
                progress->setWindowModality(Qt::WindowModal);
                progress->setMinimumDuration(2000);
            }

            qDebug() << "PENDING" << paramMgr->countPendingParams() << "PROGRESS" << pendingParams - paramMgr->countPendingParams();
            progress->setValue(pendingParams - paramMgr->countPendingParams());

            if (progress->wasCanceled()) {
                configState = CONFIG_STATE_ABORT;
                setEnabled(true);
                pendingParams = 0;
                return;
            }
        } else {
            pendingParams = 0;
            configState = CONFIG_STATE_REBOOT;
        }

        qDebug() << "PENDING PARAMS WAIT PENDING: " << paramMgr->countPendingParams();
        QTimer::singleShot(1000, this, SLOT(checkConfigState()));
        return;
    }

    if (configState == CONFIG_STATE_REBOOT) {

        // Reboot
        //TODO right now this relies upon the above send & persist finishing before the reboot command is received...

        unsigned pendingMax = 17;

        qDebug() << "PENDING PARAMS REBOOT BEFORE" << pendingParams;

        if (pendingParams == 0) {
            pendingParams = 1;

            if (progress)
                delete progress;

            progress = new QProgressDialog("Waiting for autopilot reboot", "Abort", 0, pendingMax, this);
            progress->setWindowModality(Qt::WindowModal);
            qDebug() << "Waiting for reboot, pending" << pendingParams;
        } else {
            if (progress->wasCanceled()) {
                configState = CONFIG_STATE_ABORT;
                setEnabled(true);
                pendingParams = 0;
                return;
            }
        }

        if (pendingParams == 3) {
            qDebug() << "REQUESTING REBOOT";
            mav->executeCommand(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
            mav->executeCommand(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
        }

        if (pendingParams == 4) {
            qDebug() << "DISCONNECT AIRFRAME";
            LinkManager::instance()->disconnectAll();
        }

        if (pendingParams == 14) {
            qDebug() << "CONNECT AIRFRAME";
            LinkManager::instance()->connectAll();
        }

        if (pendingParams < pendingMax) {
            progress->setValue(pendingParams);
            QTimer::singleShot(1000, this, SLOT(checkConfigState()));
        } else {
            paramMgr->requestParameterList();
            progress->setValue(pendingMax);
            configState = CONFIG_STATE_ABORT;
            pendingParams = 0;
            setEnabled(true);
            return;
        }
        qDebug() << "PENDING PARAMS REBOOT AFTER:" << pendingParams;
        pendingParams++;
        return;
    }
}

void QGCPX4AirframeConfig::setAutoConfig(bool enabled)
{
    if (!mav)
        return;
    paramMgr->setPendingParam(0, "SYS_AUTOCONFIG", (qint32) ((enabled) ? 1 : 0));
}

void QGCPX4AirframeConfig::simSelected()
{
    simSelected(ui->simComboBox->currentIndex());
}

void QGCPX4AirframeConfig::simSelected(int index)
{
    int system_index = ui->simComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::flyingWingSelected()
{
    flyingWingSelected(ui->flyingWingComboBox->currentIndex());
}

void QGCPX4AirframeConfig::flyingWingSelected(int index)
{
    int system_index = ui->flyingWingComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::planeSelected()
{
    planeSelected(ui->planeComboBox->currentIndex());
}

void QGCPX4AirframeConfig::planeSelected(int index)
{
    int system_index = ui->planeComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::quadXSelected()
{
    quadXSelected(ui->quadXComboBox->currentIndex());
}

void QGCPX4AirframeConfig::quadXSelected(int index)
{
    int system_index = ui->quadXComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::quadPlusSelected()
{
    quadPlusSelected(ui->quadPlusComboBox->currentIndex());
}

void QGCPX4AirframeConfig::quadPlusSelected(int index)
{
    int system_index = ui->quadPlusComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::hexaXSelected()
{
    hexaXSelected(ui->hexaXComboBox->currentIndex());
}

void QGCPX4AirframeConfig::hexaXSelected(int index)
{
    int system_index = ui->hexaXComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::hexaPlusSelected()
{
    hexaPlusSelected(ui->hexaPlusComboBox->currentIndex());
}

void QGCPX4AirframeConfig::hexaPlusSelected(int index)
{
    int system_index = ui->hexaPlusComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::octoXSelected()
{
    octoXSelected(ui->octoXComboBox->currentIndex());
}

void QGCPX4AirframeConfig::octoXSelected(int index)
{
    int system_index = ui->octoXComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::octoPlusSelected()
{
    octoPlusSelected(ui->octoPlusComboBox->currentIndex());
}

void QGCPX4AirframeConfig::octoPlusSelected(int index)
{
    int system_index = ui->octoPlusComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::hSelected()
{
    hSelected(ui->hComboBox->currentIndex());
}

void QGCPX4AirframeConfig::hSelected(int index)
{
    int system_index = ui->hComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}


QGCPX4AirframeConfig::~QGCPX4AirframeConfig()
{
    delete ui;
}
