#include <QMessageBox>

#include "QGCPX4AirframeConfig.h"
#include "ui_QGCPX4AirframeConfig.h"

#include "UASManager.h"
#include "UAS.h"

QGCPX4AirframeConfig::QGCPX4AirframeConfig(QWidget *parent) :
    QWidget(parent),
    mav(NULL),
    selectedId(-1),
    ui(new Ui::QGCPX4AirframeConfig)
{
    ui->setupUi(this);

    // Fill the lists here manually in accordance with the list from:
    // https://github.com/PX4/Firmware/blob/master/ROMFS/px4fmu_common/init.d/rcS

    ui->planeComboBox->addItem(tr("Multiplex Easystar 1/2"), 1);
    ui->planeComboBox->addItem(tr("Hobbyking Bixler 1/2"), 10);

    connect(ui->planeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(planeSelected(int)));

    ui->flyingWingComboBox->addItem(tr("Bormatec Camflyer Q"), 1);
    ui->flyingWingComboBox->addItem(tr("Phantom FPV"), 10);

    connect(ui->flyingWingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(flyingWingSelected(int)));

    ui->quadXComboBox->addItem(tr("Standard 10\" Quad"), 1);
    ui->quadXComboBox->addItem(tr("DJI F330 8\" Quad"), 10);

    connect(ui->quadXComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(quadXSelected(int)));

    connect(ui->quadPlusComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(quadPlusSelected(int)));

    connect(ui->hexaXComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(hexaXSelected(int)));

    connect(ui->hexaPlusComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(hexaPlusSelected(int)));

    connect(ui->octoXComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(octoXSelected(int)));

    connect(ui->octoPlusComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(octoPlusSelected(int)));

    connect(ui->hComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(hSelected(int)));

    ui->hComboBox->addItem(tr("TBS Discovery"), 15);
    ui->hComboBox->addItem(tr("H Custom"), 16);

    connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(applyAndReboot()));

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS()));

    setActiveUAS(UASManager::instance()->getActiveUAS());
}

void QGCPX4AirframeConfig::setActiveUAS(UASInterface* uas)
{
//    if (mav)

    if (!uas)
        return;

    mav = uas;

    //connect(uas->getParamManager(), SIGNAL())
}

void QGCPX4AirframeConfig::setAirframeID(int id)
{
    selectedId = id;
}

void QGCPX4AirframeConfig::applyAndReboot()
{
    // Guard against the case of an edit where we didn't receive all params yet
    if (selectedId < 0)
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

    if (mav->getParamManager()->countOnboardParams() == 0 &&
            mav->getParamManager()->countPendingParams() == 0)
    {
        mav->getParamManager()->requestParameterListIfEmpty();
        QGC::SLEEP::msleep(100);
    }

    // Guard against the case of an edit where we didn't receive all params yet
    if (mav->getParamManager()->countPendingParams() > 0)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("Parameter sync with UAS not yet complete"));
        msgBox.setInformativeText(tr("Please wait a few moments and retry"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        (void)msgBox.exec();

        return;
    }

    QList<int> components = mav->getParamManager()->getComponentForParam("SYS_AUTOSTART");

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

    qDebug() << "Setting comp" << components.first() << "SYS_AUTOSTART" << (qint32)selectedId;

    mav->getParamManager()->setParameter(components.first(), "SYS_AUTOSTART", (qint32)selectedId);

    // Send pending params
    mav->getParamManager()->sendPendingParameters();
    QGC::SLEEP::msleep(300);
    // Store parameters
    mav->getParamManager()->copyVolatileParamsToPersistent();
    QGC::SLEEP::msleep(500);
    // Reboot
    mav->executeCommand(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
}

void QGCPX4AirframeConfig::setAutoConfig(bool enabled)
{
    if (!mav)
        return;
    mav->getParamManager()->setParameter(0, "SYS_AUTOCONFIG", (qint32) ((enabled) ? 1 : 0));
}

void QGCPX4AirframeConfig::flyingWingSelected()
{

}

void QGCPX4AirframeConfig::flyingWingSelected(int index)
{
    int system_index = ui->flyingWingComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::planeSelected()
{

}

void QGCPX4AirframeConfig::planeSelected(int index)
{
    int system_index = ui->planeComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}


void QGCPX4AirframeConfig::quadXSelected()
{

}

void QGCPX4AirframeConfig::quadXSelected(int index)
{
    int system_index = ui->quadXComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::quadPlusSelected()
{

}

void QGCPX4AirframeConfig::quadPlusSelected(int index)
{
    int system_index = ui->quadPlusComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::hexaXSelected()
{

}

void QGCPX4AirframeConfig::hexaXSelected(int index)
{
    int system_index = ui->hexaXComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::hexaPlusSelected()
{

}

void QGCPX4AirframeConfig::hexaPlusSelected(int index)
{
    int system_index = ui->hexaPlusComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::octoXSelected()
{

}

void QGCPX4AirframeConfig::octoXSelected(int index)
{
    int system_index = ui->octoXComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::octoPlusSelected()
{

}

void QGCPX4AirframeConfig::octoPlusSelected(int index)
{
    int system_index = ui->octoPlusComboBox->itemData(index).toInt();
    setAirframeID(system_index);
}

void QGCPX4AirframeConfig::hSelected()
{

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
