#include "UASActionsWidget.h"
#include <QMessageBox>
#include <UAS.h>
UASActionsWidget::UASActionsWidget(QWidget *parent) : QWidget(parent)
{
    m_uas = 0;
    ui.setupUi(this);
    connect(ui.changeAltitudeButton,SIGNAL(clicked()),this,SLOT(changeAltitudeClicked()));
    connect(ui.changeSpeedButton,SIGNAL(clicked()),this,SLOT(changeSpeedClicked()));
    connect(ui.goToWaypointButton,SIGNAL(clicked()),this,SLOT(goToWaypointClicked()));
    connect(ui.armDisarmButton,SIGNAL(clicked()),this,SLOT(armButtonClicked()));
    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        activeUASSet(UASManager::instance()->getActiveUAS());
    }
}

void UASActionsWidget::activeUASSet(UASInterface *uas)
{
    m_uas = uas;
    connect(m_uas->getWaypointManager(),SIGNAL(waypointEditableListChanged()),this,SLOT(updateWaypointList()));
    connect(m_uas->getWaypointManager(),SIGNAL(currentWaypointChanged(quint16)),this,SLOT(currentWaypointChanged(quint16)));
    connect(m_uas,SIGNAL(armingChanged(bool)),this,SLOT(armingChanged(bool)));
    armingChanged(m_uas->isArmed());
    updateWaypointList();
}
void UASActionsWidget::armButtonClicked()
{
    if (m_uas)
    {
        if (m_uas->isArmed())
        {
            ((UAS*)m_uas)->disarmSystem();
        }
        else
        {
            ((UAS*)m_uas)->armSystem();
        }
    }
}

void UASActionsWidget::armingChanged(bool state)
{
    //TODO:
    //Figure out why arm/disarm is in UAS.h and not part of the interface, and fix.
    if (state)
    {
        ui.armDisarmButton->setText("DISARM\nCurrently Armed");
    }
    else
    {
        ui.armDisarmButton->setText("ARM\nCurrently Disarmed");
    }

}

void UASActionsWidget::currentWaypointChanged(quint16 wpid)
{
    ui.currentWaypointLabel->setText("Current: " + QString::number(wpid));
}

void UASActionsWidget::updateWaypointList()
{
    ui.waypointComboBox->clear();
    for (int i=0;i<m_uas->getWaypointManager()->getWaypointEditableList().size();i++)
    {
        ui.waypointComboBox->addItem(QString::number(i));
    }
}

UASActionsWidget::~UASActionsWidget()
{
}
void UASActionsWidget::goToWaypointClicked()
{
    if (!m_uas)
    {
        return;
    }
    m_uas->getWaypointManager()->setCurrentWaypoint(ui.waypointComboBox->currentIndex());
}

void UASActionsWidget::changeAltitudeClicked()
{
    QMessageBox::information(0,"Error","No implemented yet.");
}

void UASActionsWidget::changeSpeedClicked()
{
    if (!m_uas)
    {
        return;
    }
    if (m_uas->getSystemType() == MAV_TYPE_QUADROTOR)
    {
        m_uas->setParameter(1,"WP_SPEED_MAX",QVariant(((float)ui.altitudeSpinBox->value() * 100)));
        return;
    }
    else if (m_uas->getSystemType() == MAV_TYPE_FIXED_WING)
    {
        QVariant variant;
        if (m_uas->getParamManager()->getParameterValue(1,"ARSPD_ENABLE",variant))
        {
            if (variant.toInt() == 1)
            {
                m_uas->setParameter(1,"TRIM_ARSPD_CN",QVariant(((float)ui.altitudeSpinBox->value() * 100)));
                return;
            }

        }
        m_uas->setParameter(1,"TRIM_ARSPD_CN",QVariant(((float)ui.altitudeSpinBox->value() * 100)));
    }
}
