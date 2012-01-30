/*===================================================================
======================================================================*/

/**
 * @file
 *   @brief Displays one waypoint
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Benjamin Knecht <mavteam@student.ethz.ch>
 *   @author Petri Tanskanen <mavteam@student.ethz.ch>
 *
 */

#include <QDoubleSpinBox>
#include <QDebug>

#include <cmath>
#include <qmath.h>

#include "WaypointEditableView.h"
#include "ui_WaypointEditableView.h"
#include "ui_QGCCustomWaypointAction.h"
#include "ui_QGCMissionDoWidget.h"
#include "ui_QGCMissionOther.h"

#include "QGCMissionNavWaypoint.h"
#include "QGCMissionDoWidget.h"
#include "QGCMissionConditionWidget.h"
#include "QGCMissionOther.h"


WaypointEditableView::WaypointEditableView(Waypoint* wp, QWidget* parent) :
    QWidget(parent),
    viewMode(QGC_WAYPOINTEDITABLEVIEW_MODE_DEFAULT),
    m_ui(new Ui::WaypointEditableView)
{
    m_ui->setupUi(this);

    this->wp = wp;
    connect(wp, SIGNAL(destroyed(QObject*)), this, SLOT(deleted(QObject*)));

    // CUSTOM COMMAND WIDGET
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(2);
    layout->setContentsMargins(4, 4 ,4 ,4);
    m_ui->customActionWidget->setLayout(layout);

    MissionNavWaypointWidget = NULL;
    MissionDoJumpWidget = NULL;
    MissionConditionDelayWidget = NULL;
    MissionOtherWidget = NULL;


    // add actions
    m_ui->comboBox_action->addItem(tr("NAV: Waypoint"),MAV_CMD_NAV_WAYPOINT);
    m_ui->comboBox_action->addItem(tr("NAV: TakeOff"),MAV_CMD_NAV_TAKEOFF);
    m_ui->comboBox_action->addItem(tr("NAV: Loiter Unlim."),MAV_CMD_NAV_LOITER_UNLIM);
    m_ui->comboBox_action->addItem(tr("NAV: Loiter Time"),MAV_CMD_NAV_LOITER_TIME);
    m_ui->comboBox_action->addItem(tr("NAV: Loiter Turns"),MAV_CMD_NAV_LOITER_TURNS);
    m_ui->comboBox_action->addItem(tr("NAV: Ret. to Launch"),MAV_CMD_NAV_RETURN_TO_LAUNCH);
    m_ui->comboBox_action->addItem(tr("NAV: Land"),MAV_CMD_NAV_LAND);
    //m_ui->comboBox_action->addItem(tr("NAV: Target"),MAV_CMD_NAV_TARGET);
    m_ui->comboBox_action->addItem(tr("IF: Delay over"),MAV_CMD_CONDITION_DELAY);
    //m_ui->comboBox_action->addItem(tr("IF: Yaw angle is"),MAV_CMD_CONDITION_YAW);
    m_ui->comboBox_action->addItem(tr("DO: Jump to Index"),MAV_CMD_DO_JUMP);
    m_ui->comboBox_action->addItem(tr("Other"), MAV_CMD_ENUM_END);

    // add frames
    m_ui->comboBox_frame->addItem("Global/Abs. Alt",MAV_FRAME_GLOBAL);
    m_ui->comboBox_frame->addItem("Global/Rel. Alt", MAV_FRAME_GLOBAL_RELATIVE_ALT);
    m_ui->comboBox_frame->addItem("Local(NED)",MAV_FRAME_LOCAL_NED);
    m_ui->comboBox_frame->addItem("Mission",MAV_FRAME_MISSION);

    // Initialize view correctly
    updateActionView(wp->getAction());    

    // Read values and set user interface
    updateValues();

    // Check for mission frame
    if (wp->getFrame() == MAV_FRAME_MISSION)
    {
        m_ui->comboBox_action->setCurrentIndex(m_ui->comboBox_action->count()-1);
    }

    connect(m_ui->upButton, SIGNAL(clicked()), this, SLOT(moveUp()));
    connect(m_ui->downButton, SIGNAL(clicked()), this, SLOT(moveDown()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(remove()));

    connect(m_ui->autoContinue, SIGNAL(stateChanged(int)), this, SLOT(changedAutoContinue(int)));
    connect(m_ui->selectedBox, SIGNAL(stateChanged(int)), this, SLOT(changedCurrent(int)));
    connect(m_ui->comboBox_action, SIGNAL(activated(int)), this, SLOT(changedAction(int)));
    connect(m_ui->comboBox_frame, SIGNAL(activated(int)), this, SLOT(changedFrame(int)));

}

void WaypointEditableView::moveUp()
{
    emit moveUpWaypoint(wp);
}

void WaypointEditableView::moveDown()
{
    emit moveDownWaypoint(wp);
}


void WaypointEditableView::remove()
{
    emit removeWaypoint(wp);
    deleteLater();
}

void WaypointEditableView::changedAutoContinue(int state)
{
    if (state == 0)
        wp->setAutocontinue(false);
    else
        wp->setAutocontinue(true);
}

void WaypointEditableView::updateActionView(int action)
{
    //Hide all
    if(MissionNavWaypointWidget) MissionNavWaypointWidget->hide();
    if(MissionOtherWidget) MissionOtherWidget->hide();
    if(MissionConditionDelayWidget) MissionConditionDelayWidget->hide();
    if(MissionDoJumpWidget) MissionDoJumpWidget->hide();

    //Show only the correct one
    if (viewMode != QGC_WAYPOINTEDITABLEVIEW_MODE_DIRECT_EDITING)
    {
        switch(action) {
        case MAV_CMD_NAV_TAKEOFF:
        case MAV_CMD_NAV_LAND:
        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
        case MAV_CMD_NAV_LOITER_UNLIM:
        case MAV_CMD_NAV_LOITER_TURNS:
        case MAV_CMD_NAV_LOITER_TIME:
            break;
        case MAV_CMD_NAV_WAYPOINT:
            if(MissionNavWaypointWidget) MissionNavWaypointWidget->show();
            break;
        case MAV_CMD_DO_JUMP:
            if(MissionDoJumpWidget) MissionDoJumpWidget->show();
            break;
        default:
            if(MissionOtherWidget) MissionOtherWidget->show();
            break;
        }
    }
    else
    {
        if(MissionOtherWidget) MissionOtherWidget->show();
    }
}

/**
 * @param index The index of the combo box of the action entry, NOT the action ID
 */
void WaypointEditableView::changedAction(int index)
{
    MAV_FRAME cur_frame = (MAV_FRAME) m_ui->comboBox_frame->itemData(m_ui->comboBox_frame->currentIndex()).toUInt();
    // set waypoint action
    int actionIndex = m_ui->comboBox_action->itemData(index).toUInt();
    if (actionIndex < MAV_CMD_ENUM_END && actionIndex >= 0) {
        MAV_CMD action = (MAV_CMD) actionIndex;
        wp->setAction(action);
    }

    // Expose ui based on action
    // Change to mission frame
    // if action is unknown
    viewMode = QGC_WAYPOINTEDITABLEVIEW_MODE_DEFAULT;
    switch(actionIndex) {        
    case MAV_CMD_NAV_TAKEOFF:
    case MAV_CMD_NAV_LAND:
    case MAV_CMD_NAV_RETURN_TO_LAUNCH:    
    case MAV_CMD_NAV_LOITER_UNLIM:
    case MAV_CMD_NAV_LOITER_TURNS:
    case MAV_CMD_NAV_LOITER_TIME:
        updateActionView(actionIndex);
        break;

    case MAV_CMD_NAV_WAYPOINT:
        if (!MissionNavWaypointWidget)
        {
            MissionNavWaypointWidget = new QGCMissionNavWaypoint(this);
            m_ui->customActionWidget->layout()->addWidget(MissionNavWaypointWidget);
        }
        updateActionView(actionIndex);
        break;

    case MAV_CMD_DO_JUMP:        
        if (!MissionDoJumpWidget)
        {
            MissionDoJumpWidget = new QGCMissionDoWidget(this);
            m_ui->customActionWidget->layout()->addWidget(MissionDoJumpWidget);
        }
        updateActionView(actionIndex);
        break;

    case MAV_CMD_ENUM_END:
    default:
        // Switch to mission frame
        viewMode = QGC_WAYPOINTEDITABLEVIEW_MODE_DIRECT_EDITING;
        if (!MissionOtherWidget)
        {
            MissionOtherWidget = new QGCMissionOther(this);
            m_ui->customActionWidget->layout()->addWidget(MissionOtherWidget);
        }
        updateActionView(actionIndex);
        break;
    }
    updateValues();
}

void WaypointEditableView::deleted(QObject* waypoint)
{
    Q_UNUSED(waypoint);
}

void WaypointEditableView::changedFrame(int index)
{
    // set waypoint action
    MAV_FRAME frame = (MAV_FRAME)m_ui->comboBox_frame->itemData(index).toUInt();
    wp->setFrame(frame);
}

void WaypointEditableView::changedCurrent(int state)
{    
    if (state == 0)
    {
        if (wp->getCurrent() == true) //User clicked on the waypoint, that is already current
        {            
            m_ui->selectedBox->setChecked(true);
            m_ui->selectedBox->setCheckState(Qt::Checked);
        }
        else
        {            
            m_ui->selectedBox->setChecked(false);
            m_ui->selectedBox->setCheckState(Qt::Unchecked);            
        }
    }
    else
    {       
        wp->setCurrent(true);
        emit changeCurrentWaypoint(wp->getId());   //the slot changeCurrentWaypoint() in WaypointList sets all other current flags to false
    }    
}

void WaypointEditableView::updateValues()
{
    // Check if we just lost the wp, delete the widget
    // accordingly
    if (!wp) {
        deleteLater();
        return;
    }

    //wp->blockSignals(true);

    // Deactivate all QDoubleSpinBox signals due to
    // unwanted rounding effects
    for (int j = 0; j  < children().size(); ++j)
    {
        // Store only QGCToolWidgetItems
        QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(children().at(j));
        if (spin)
        {
            //qDebug() << "DEACTIVATED SPINBOX #" << j;
            spin->blockSignals(true);
        }
        else
        {
            // Store only QGCToolWidgetItems
            QWidget* item = dynamic_cast<QWidget*>(children().at(j));
            if (item)
            {
                //qDebug() << "FOUND WIDGET BOX";
                for (int k = 0; k  < item->children().size(); ++k)
                {
                    // Store only QGCToolWidgetItems
                    QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(item->children().at(k));
                    if (spin)
                    {
                        //qDebug() << "DEACTIVATED SPINBOX #" << k;
                        spin->blockSignals(true);
                    }
                }
            }
        }
    }

    // Block all custom action widget actions
    for (int j = 0; j  < m_ui->customActionWidget->children().size(); ++j)
    {
        // Store only QGCToolWidgetItems
        QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(m_ui->customActionWidget->children().at(j));
        if (spin)
        {
            qDebug() << "DEACTIVATED SPINBOX #" << j;
            spin->blockSignals(true);
        }
        else
        {
            // Store only QGCToolWidgetItems
            QWidget* item = dynamic_cast<QWidget*>(m_ui->customActionWidget->children().at(j));
            if (item)
            {
                //qDebug() << "CUSTOM ACTIONS FOUND WIDGET BOX";
                for (int k = 0; k  < item->children().size(); ++k)
                {
                    // Store only QGCToolWidgetItems
                    QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(item->children().at(k));
                    if (spin)
                    {
                        //qDebug() << "DEACTIVATED SPINBOX #" << k;
                        spin->blockSignals(true);
                    }
                }
            }
        }
    }


    // update frame
    MAV_FRAME frame = wp->getFrame();
    int frame_index = m_ui->comboBox_frame->findData(frame);
    if (m_ui->comboBox_frame->currentIndex() != frame_index) {
        m_ui->comboBox_frame->setCurrentIndex(frame_index);        
    }

    // Update action
    MAV_CMD action = wp->getAction();
    int action_index = m_ui->comboBox_action->findData(action);
    // Set to "Other" action if it was -1
    if (action_index == -1)
    {
        action_index = m_ui->comboBox_action->findData(MAV_CMD_ENUM_END);
    }

    // Only update if changed
    if (m_ui->comboBox_action->currentIndex() != action_index)
    {
        // If action is unknown, set direct editing mode
        if (wp->getAction() < 0 || wp->getAction() > MAV_CMD_NAV_TAKEOFF)//FIXME condition should not depend on NAV_TAKEOFF
        {
            //changeViewMode(QGC_WAYPOINTEDITABLEVIEW_MODE_DIRECT_EDITING);
        }
        else
        {
            if (viewMode != QGC_WAYPOINTEDITABLEVIEW_MODE_DIRECT_EDITING)
            {
                // Action ID known, update
                m_ui->comboBox_action->setCurrentIndex(action_index);
                updateActionView(action);                
            }
        }
    }


    emit commandBroadcast(wp->getAction());
    emit frameBroadcast(wp->getFrame());
    emit param1Broadcast(wp->getParam1());
    emit param2Broadcast(wp->getParam2());
    emit param3Broadcast(wp->getParam3());
    emit param4Broadcast(wp->getParam4());
    emit param5Broadcast(wp->getParam5());
    emit param6Broadcast(wp->getParam6());
    emit param7Broadcast(wp->getParam7());


    if (m_ui->selectedBox->isChecked() != wp->getCurrent())
    {
        m_ui->selectedBox->setChecked(wp->getCurrent());
    }
    if (m_ui->autoContinue->isChecked() != wp->getAutoContinue())
    {
        m_ui->autoContinue->setChecked(wp->getAutoContinue());
    }
    m_ui->idLabel->setText(QString("%1").arg(wp->getId()));



    QColor backGroundColor = QGC::colorBackground;

    static int lastId = -1;
    int currId = wp->getId() % 2;

    if (currId != lastId)
    {

        // qDebug() << "COLOR ID: " << currId;
        if (currId == 1)
        {
            //backGroundColor = backGroundColor.lighter(150);
            backGroundColor = QColor("#252528").lighter(150);
        }
        else
        {
            backGroundColor = QColor("#252528").lighter(250);
        }
        // qDebug() << "COLOR:" << backGroundColor.name();

        // Update color based on id
        QString groupBoxStyle = QString("QGroupBox {padding: 0px; margin: 0px; border: 0px; background-color: %1; }").arg(backGroundColor.name());
        QString labelStyle = QString("QWidget {background-color: %1; color: #DDDDDF; border-color: #EEEEEE; }").arg(backGroundColor.name());
        QString checkBoxStyle = QString("QCheckBox {background-color: %1; color: #454545; border-color: #EEEEEE; }").arg(backGroundColor.name());
        QString widgetSlotStyle = QString("QWidget {background-color: %1; color: #DDDDDF; border-color: #EEEEEE; } QSpinBox {background-color: #252528 } QDoubleSpinBox {background-color: #252528 } QComboBox {background-color: #252528 }").arg(backGroundColor.name()); //FIXME There should be a way to declare background color for widgetSlot without letting the children inherit this color. Here, background color for every widget-type (QSpinBox, etc.) has to be declared separately to overrule the coloring of QWidget.

        m_ui->autoContinue->setStyleSheet(checkBoxStyle);
        m_ui->selectedBox->setStyleSheet(checkBoxStyle);
        m_ui->idLabel->setStyleSheet(labelStyle);
        m_ui->groupBox->setStyleSheet(groupBoxStyle);
        m_ui->customActionWidget->setStyleSheet(widgetSlotStyle);
        lastId = currId;
    }

    // Activate all QDoubleSpinBox signals due to
    // unwanted rounding effects
    for (int j = 0; j  < children().size(); ++j)
    {
        // Store only QGCToolWidgetItems
        QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(children().at(j));
        if (spin)
        {
            //qDebug() << "ACTIVATED SPINBOX #" << j;
            spin->blockSignals(false);
        }
        else
        {
            // Store only QGCToolWidgetItems
            QGroupBox* item = dynamic_cast<QGroupBox*>(children().at(j));
            if (item)
            {
                //qDebug() << "FOUND GROUP BOX";
                for (int k = 0; k  < item->children().size(); ++k)
                {
                    // Store only QGCToolWidgetItems
                    QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(item->children().at(k));
                    if (spin)
                    {
                        //qDebug() << "ACTIVATED SPINBOX #" << k;
                        spin->blockSignals(false);
                    }
                }
            }
        }
    }

    // Unblock all custom action widget actions
    for (int j = 0; j  < m_ui->customActionWidget->children().size(); ++j)
    {
        // Store only QGCToolWidgetItems
        QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(m_ui->customActionWidget->children().at(j));
        if (spin)
        {
            //qDebug() << "ACTIVATED SPINBOX #" << j;
            spin->blockSignals(false);
        }
        else
        {
            // Store only QGCToolWidgetItems
            QWidget* item = dynamic_cast<QWidget*>(m_ui->customActionWidget->children().at(j));
            if (item)
            {
                //qDebug() << "FOUND WIDGET BOX";
                for (int k = 0; k  < item->children().size(); ++k)
                {
                    // Store only QGCToolWidgetItems
                    QDoubleSpinBox* spin = dynamic_cast<QDoubleSpinBox*>(item->children().at(k));
                    if (spin)
                    {
                       //qDebug() << "ACTIVATED SPINBOX #" << k;
                        spin->blockSignals(false);
                    }
                }
            }
        }
    }

//    wp->blockSignals(false);
}

void WaypointEditableView::setCurrent(bool state)
{
    m_ui->selectedBox->blockSignals(true);
    m_ui->selectedBox->setChecked(state);
    m_ui->selectedBox->blockSignals(false);
}


void WaypointEditableView::changedCommand(int mav_cmd_id)
{
    if (mav_cmd_id<MAV_CMD_ENUM_END)
    {
        wp->setAction(mav_cmd_id);
    }
}
void WaypointEditableView::changedParam1(double value)
{
    wp->setParam1(value);
}
void WaypointEditableView::changedParam2(double value)
{
    wp->setParam2(value);
}
void WaypointEditableView::changedParam3(double value)
{
    wp->setParam3(value);
}
void WaypointEditableView::changedParam4(double value)
{
    wp->setParam4(value);
}
void WaypointEditableView::changedParam5(double value)
{
    wp->setParam5(value);
}
void WaypointEditableView::changedParam6(double value)
{
    wp->setParam6(value);
}
void WaypointEditableView::changedParam7(double value)
{
    wp->setParam7(value);
}

WaypointEditableView::~WaypointEditableView()
{
    delete m_ui;
}

void WaypointEditableView::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
