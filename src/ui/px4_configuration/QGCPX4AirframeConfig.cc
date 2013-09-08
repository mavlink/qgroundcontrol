#include <QMessageBox>
#include <QProgressDialog>
#include <QDebug>
#include <QTimer>

#include "QGCPX4AirframeConfig.h"
#include "ui_QGCPX4AirframeConfig.h"

#include "UASManager.h"
#include "LinkManager.h"
#include "UAS.h"
#include "QGC.h"

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

    ui->planeComboBox->addItem(tr("Multiplex Easystar 1/2"), 100);
    ui->planeComboBox->addItem(tr("Hobbyking Bixler 1/2"), 101);
    ui->planeComboBox->addItem(tr("HilStar (SIMULATION)"), 1000);

    connect(ui->planePushButton, SIGNAL(clicked()), this, SLOT(planeSelected()));
    connect(ui->planeComboBox, SIGNAL(activated(int)), this, SLOT(planeSelected(int)));
    ui->planePushButton->setEnabled(ui->planeComboBox->count() > 0);

    ui->flyingWingComboBox->addItem(tr("Bormatec Camflyer Q"), 30);
    ui->flyingWingComboBox->addItem(tr("Phantom FPV"), 31);

    connect(ui->flyingWingPushButton, SIGNAL(clicked()), this, SLOT(flyingWingSelected()));
    connect(ui->flyingWingComboBox, SIGNAL(activated(int)), this, SLOT(flyingWingSelected(int)));

    ui->quadXComboBox->addItem(tr("DJI F330 8\" Quad"), 10);
    ui->quadXComboBox->addItem(tr("DJI F450 10\" Quad"), 11);
    ui->quadXComboBox->addItem(tr("Turnigy Talon v2 X550 Quad"), 666);
    ui->quadXComboBox->addItem(tr("AR.Drone Frame Quad"), 8);
    ui->quadXComboBox->addItem(tr("AR.Drone Quad (w. PX4FLOW)"), 9);

    connect(ui->quadXPushButton, SIGNAL(clicked()), this, SLOT(quadXSelected()));
    connect(ui->quadXComboBox, SIGNAL(activated(int)), this, SLOT(quadXSelected(int)));

    connect(ui->quadPlusPushButton, SIGNAL(clicked()), this, SLOT(quadPlusSelected()));
    connect(ui->quadPlusComboBox, SIGNAL(activated(int)), this, SLOT(quadPlusSelected(int)));
    ui->quadPlusPushButton->setEnabled(ui->quadPlusComboBox->count() > 0);

    connect(ui->hexaXPushButton, SIGNAL(clicked()), this, SLOT(hexaXSelected()));
    connect(ui->hexaXComboBox, SIGNAL(activated(int)), this, SLOT(hexaXSelected(int)));
    ui->hexaXPushButton->setEnabled(ui->hexaXComboBox->count() > 0);

    connect(ui->hexaPlusPushButton, SIGNAL(clicked()), this, SLOT(hexaPlusSelected()));
    connect(ui->hexaPlusComboBox, SIGNAL(activated(int)), this, SLOT(hexaPlusSelected(int)));
    ui->hexaPlusPushButton->setEnabled(ui->hexaPlusComboBox->count() > 0);

    connect(ui->octoXPushButton, SIGNAL(clicked()), this, SLOT(octoXSelected()));
    connect(ui->octoXComboBox, SIGNAL(activated(int)), this, SLOT(octoXSelected(int)));
    ui->octoXPushButton->setEnabled(ui->octoXComboBox->count() > 0);

    connect(ui->octoPlusPushButton, SIGNAL(clicked()), this, SLOT(octoPlusSelected()));
    connect(ui->octoPlusComboBox, SIGNAL(activated(int)), this, SLOT(octoPlusSelected(int)));
    ui->octoPlusPushButton->setEnabled(ui->octoPlusComboBox->count() > 0);

    ui->hComboBox->addItem(tr("3DR Iris"), 16);
    ui->hComboBox->addItem(tr("TBS Discovery"), 15);

    connect(ui->hPushButton, SIGNAL(clicked()), this, SLOT(hSelected()));
    connect(ui->hComboBox, SIGNAL(activated(int)), this, SLOT(hSelected(int)));
    ui->hPushButton->setEnabled(ui->hComboBox->count() > 0);

    connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(applyAndReboot()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    setActiveUAS(UASManager::instance()->getActiveUAS());

    uncheckAll();
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

    if (id > 0 && id < 15) {
        ui->quadXPushButton->setChecked(true);
        ui->quadXComboBox->setCurrentIndex(ui->quadXComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected quad X (ID: #%1)").arg(selectedId));
    }
    else if (id >= 15 && id < 20)
    {
        ui->hPushButton->setChecked(true);
        ui->hComboBox->setCurrentIndex(ui->hComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected H Frame (ID: #%1)").arg(selectedId));
    }
    else if (id >= 30 && id < 50)
    {
        ui->flyingWingPushButton->setChecked(true);
        ui->flyingWingComboBox->setCurrentIndex(ui->flyingWingComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected flying wing (ID: #%1)").arg(selectedId));
    }
    else if (id >= 100 && id < 150)
    {
        ui->planePushButton->setChecked(true);
        ui->planeComboBox->setCurrentIndex(ui->planeComboBox->findData(id));
        ui->statusLabel->setText(tr("Selected plane (ID: #%1)").arg(selectedId));
    }
}

void QGCPX4AirframeConfig::applyAndReboot()
{
    // Guard against the case of an edit where we didn't receive all params yet
    if (selectedId <= 0)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("No airframe selected"));
        msgBox.setInformativeText(tr("Please select an airframe first."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        (void)msgBox.exec();

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
        QMessageBox msgBox;
        msgBox.setText(tr("Parameter sync with UAS not yet complete"));
        msgBox.setInformativeText(tr("Please wait a few moments and retry"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        (void)msgBox.exec();

        return;
    }

    // Guard against multiple components responding - this will never show in practice
    if (components.count() != 1) {
        QMessageBox msgBox;
        msgBox.setText(tr("Invalid system setup detected"));
        msgBox.setInformativeText(tr("None or more than one component advertised to provide the main system configuration option. This is an invalid system setup - please check your autopilot."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        (void)msgBox.exec();

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

        unsigned pendingMax = 20;

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
        if (pendingParams == 15) {
            qDebug() << "DISCONNECT AIRFRAME";
            LinkManager::instance()->disconnectAll();
        }
        if (pendingParams == 16) {
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
