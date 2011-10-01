#include <QDebug>
#include "WaypointViewOnlyView.h"
#include "ui_WaypointViewOnlyView.h"

WaypointViewOnlyView::WaypointViewOnlyView(Waypoint* wp, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::WaypointViewOnlyView)
{
    m_ui->setupUi(this);
    this->wp = wp;
    updateValues();

    connect(m_ui->current, SIGNAL(stateChanged(int)), this, SLOT(changedCurrent(int)));
    connect(m_ui->autoContinue, SIGNAL(stateChanged(int)),this, SLOT(changedAutoContinue(int)));
}

void WaypointViewOnlyView::changedAutoContinue(int state)
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

void WaypointViewOnlyView::changedCurrent(int state)
{
    qDebug() << "Trof: WaypointViewOnlyView::changedCurrent(" << state << ") ID:" << wp->getId();
    m_ui->current->blockSignals(true);
    if (state == 0)
    {
        /*

        m_ui->current->setStyleSheet("");

        */
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
        /*
        FIXME: The checkbox should turn gray to indicate, that set_current request has been sent to UAV. It should become blue (checked) after receiving set_current_ack from waypointplanner.

        m_ui->current->setStyleSheet("*::indicator { \
            border: 1px solid #777777; \
            border-radius: 2px; \
            color: #999999; \
                 width: 10px; \
             height: 10px; \
        }");
        */
        wp->setCurrent(true);
        emit changeCurrentWaypoint(wp->getId());   //the slot changeCurrentWaypoint() in WaypointList sets all other current flags to false
    }
    m_ui->current->blockSignals(false);
}

void WaypointViewOnlyView::setCurrent(bool state)
{
    m_ui->current->blockSignals(true);
    m_ui->current->setChecked(state);
    m_ui->current->blockSignals(false);
}

void WaypointViewOnlyView::updateValues()
{
    qDebug() << "Trof: WaypointViewOnlyView::updateValues() ID:" << wp->getId();
    // Check if we just lost the wp, delete the widget
    // accordingly
    if (!wp)
    {
        deleteLater();
        return;
    }

    // Update style

    QColor backGroundColor = QGC::colorBackground;

    static int lastId = -1;
    int currId = wp->getId() % 2;

    if (currId != lastId)
    {

        // qDebug() << "COLOR ID: " << currId;
        if (currId == 1)
        {
            backGroundColor = QColor("#252528").lighter(150);
        }
        else
        {
            backGroundColor = QColor("#252528").lighter(250);
        }

        // Update color based on id
        QString groupBoxStyle = QString("QGroupBox {padding: 0px; margin: 0px; border: 0px; background-color: %1; min-height: 12px; }").arg(backGroundColor.name());
        QString labelStyle = QString("QWidget {background-color: %1; color: #DDDDDF; border-color: #EEEEEE; }").arg(backGroundColor.name());
        QString displayBarStyle = QString("QWidget {background-color: %1; color: #DDDDDF; border: none; }").arg(backGroundColor.name());
        QString checkBoxStyle = QString("QCheckBox {background-color: %1; color: #454545; border-color: #EEEEEE; }").arg(backGroundColor.name());

        m_ui->autoContinue->setStyleSheet(checkBoxStyle);
        m_ui->current->setStyleSheet(checkBoxStyle);
        m_ui->idLabel->setStyleSheet(labelStyle);
        m_ui->displayBar->setStyleSheet(displayBarStyle);
        m_ui->groupBox->setStyleSheet(groupBoxStyle);
        lastId = currId;
    }

    // update frame

    m_ui->idLabel->setText(QString("%1").arg(wp->getId()));

    if (m_ui->current->isChecked() != wp->getCurrent())
    {
        m_ui->current->blockSignals(true);
        m_ui->current->setChecked(wp->getCurrent());
        m_ui->current->blockSignals(false);
    }
    if (m_ui->autoContinue->isChecked() != wp->getAutoContinue())
    {
        m_ui->autoContinue->blockSignals(true);
        m_ui->autoContinue->setChecked(wp->getAutoContinue());
        m_ui->autoContinue->blockSignals(false);
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
}

WaypointViewOnlyView::~WaypointViewOnlyView()
{
    delete m_ui;
}
