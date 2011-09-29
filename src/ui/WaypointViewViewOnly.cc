#include <QDebug>
#include "WaypointViewViewOnly.h"
#include "ui_WaypointViewViewOnly.h"

WaypointViewViewOnly::WaypointViewViewOnly(Waypoint* wp, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::WaypointViewViewOnly)
{
    m_ui->setupUi(this);
    this->wp = wp;
    updateValues();

    connect(m_ui->current, SIGNAL(stateChanged(int)), this, SLOT(changedCurrent(int)));
    connect(m_ui->autoContinue, SIGNAL(stateChanged(int)),this, SLOT(changedAutoContinue(int)));
}

void WaypointViewViewOnly::changedAutoContinue(int state)
{
    bool new_value = false;
    if (state != 0)
    {
        new_value = true;
    }
    m_ui->autoContinue->blockSignals(true);
    m_ui->autoContinue->setChecked(state);
    m_ui->autoContinue->blockSignals(false);
    wp->setAutocontinue(new_value);
    emit changeAutoContinue(wp->getId(),new_value);
 }

void WaypointViewViewOnly::changedCurrent(int state)
{
    qDebug() << "Trof: WaypointViewViewOnly::changedCurrent(" << state << ") ID:" << wp->getId();

    if (state == 0)
    {
        if (wp->getCurrent() == true) //User clicked on the waypoint, that is already current
        {
            m_ui->current->setChecked(true);
            m_ui->current->setCheckState(Qt::Checked);
        }
        else
        {
            m_ui->current->setChecked(false);
            m_ui->current->setCheckState(Qt::Unchecked);
            wp->setCurrent(false);
        }
    }
    else
    {
        wp->setCurrent(true);
        emit changeCurrentWaypoint(wp->getId());   //the slot changeCurrentWaypoint() in WaypointList sets all other current flags to false
    }
}

void WaypointViewViewOnly::setCurrent(bool state)
{
    m_ui->current->blockSignals(true);
    m_ui->current->setChecked(state);
    m_ui->current->blockSignals(false);
}

void WaypointViewViewOnly::updateValues()
{
    qDebug() << "Trof: WaypointViewViewOnly::updateValues() ID:" << wp->getId();
    // Check if we just lost the wp, delete the widget
    // accordingly
    if (!wp)
    {
        deleteLater();
        return;
    }
    // Deactivate signals from the WP
    wp->blockSignals(true);
    // update frame

    m_ui->idLabel->setText(QString("%1").arg(wp->getId()));

    if (m_ui->current->isChecked() != wp->getCurrent())
    {
        m_ui->current->setChecked(wp->getCurrent());
    }
    if (m_ui->autoContinue->isChecked() != wp->getAutoContinue())
    {
        m_ui->autoContinue->setChecked(wp->getAutoContinue());
    }

switch (wp->getAction())
{
case MAV_CMD_NAV_WAYPOINT:
    {
        if (wp->getParam1()>0)
        {
            m_ui->displayBar->setText(QString("Go to (%1, %2, %3) and wait there for %4 sec; yaw: %5; rad: %6").arg(wp->getX()).arg(wp->getY()).arg(wp->getZ()).arg(wp->getParam1()).arg(wp->getParam4()).arg(wp->getParam2()));
        }
        else
        {
            m_ui->displayBar->setText(QString("Go to (%1, %2, %3); yaw: %4; rad: %5").arg(wp->getX()).arg(wp->getY()).arg(wp->getZ()).arg(wp->getParam4()).arg(wp->getParam2()));
        }
        break;
    }
case MAV_CMD_NAV_LAND:
    {
        m_ui->displayBar->setText(QString("LAND. Go to (%1, %2, %3) and descent; yaw: %4").arg(wp->getX()).arg(wp->getY()).arg(wp->getZ()).arg(wp->getParam4()));
        break;
    }
case MAV_CMD_NAV_TAKEOFF:
    {
        m_ui->displayBar->setText(QString("TAKEOFF. Go to (%1, %2, %3); yaw: %4").arg(wp->getX()).arg(wp->getY()).arg(wp->getZ()).arg(wp->getParam4()));
        break;
    }
case MAV_CMD_DO_JUMP:
    {
        if (wp->getParam2()>0)
        {
            m_ui->displayBar->setText(QString("Jump to waypoint %1. Jumps left: %2").arg(wp->getParam1()).arg(wp->getParam2()));
        }
        else
        {
            m_ui->displayBar->setText(QString("No jumps left. Proceed to next waypoint."));
        }
        break;
    }
case MAV_CMD_CONDITION_DELAY:
    {
        m_ui->displayBar->setText(QString("Delay: %1 sec").arg(wp->getParam1()));
        break;
    }
case 237: //MAV_CMD_DO_START_SEARCH
    {
        m_ui->displayBar->setText(QString("Start searching for pattern. Success when got more than %2 detections with confidence %1").arg(wp->getParam1()).arg(wp->getParam2()));
        break;
    }
case 238: //MAV_CMD_DO_FINISH_SEARCH
    {
        m_ui->displayBar->setText(QString("Check if search was successful. yes -> jump to %1, no -> jump to %2.  Jumps left: %3").arg(wp->getParam1()).arg(wp->getParam2()).arg(wp->getParam3()));
        break;
    }
case 240: //MAV_CMD_DO_SWEEP
    {
        m_ui->displayBar->setText(QString("Sweep. Corners: (%1,%2) and (%3,%4); z: %5; scan radius: %6").arg(wp->getParam3()).arg(wp->getParam4()).arg(wp->getParam5()).arg(wp->getParam6()).arg(wp->getParam7()).arg(wp->getParam1()));
        break;
    }
default:
    {
        m_ui->displayBar->setText(QString("Unknown Command ID (%1) : %2, %3, %4, %5, %6, %7, %8").arg(wp->getAction()).arg(wp->getParam1()).arg(wp->getParam2()).arg(wp->getParam3()).arg(wp->getParam4()).arg(wp->getParam5()).arg(wp->getParam6()).arg(wp->getParam7()));
        break;
    }
}


    wp->blockSignals(false);
}

WaypointViewViewOnly::~WaypointViewViewOnly()
{
    delete m_ui;
}
