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

#include "WaypointView.h"
#include "ui_WaypointView.h"
#include "ui_QGCCustomWaypointAction.h"

WaypointView::WaypointView(Waypoint* wp, QWidget* parent) :
    QWidget(parent),
    customCommand(new Ui_QGCCustomWaypointAction),
    viewMode(QGC_WAYPOINTVIEW_MODE_NAV),
    m_ui(new Ui::WaypointView)
{
    m_ui->setupUi(this);

    this->wp = wp;
    connect(wp, SIGNAL(destroyed(QObject*)), this, SLOT(deleted(QObject*)));

    // CUSTOM COMMAND WIDGET
    customCommand->setupUi(m_ui->customActionWidget);

    // add actions
    m_ui->comboBox_action->addItem(tr("NAV: Waypoint"),MAV_CMD_NAV_WAYPOINT);
    m_ui->comboBox_action->addItem(tr("NAV: TakeOff"),MAV_CMD_NAV_TAKEOFF);
    m_ui->comboBox_action->addItem(tr("NAV: Loiter Unlim."),MAV_CMD_NAV_LOITER_UNLIM);
    m_ui->comboBox_action->addItem(tr("NAV: Loiter Time"),MAV_CMD_NAV_LOITER_TIME);
    m_ui->comboBox_action->addItem(tr("NAV: Loiter Turns"),MAV_CMD_NAV_LOITER_TURNS);
    m_ui->comboBox_action->addItem(tr("NAV: Ret. to Launch"),MAV_CMD_NAV_RETURN_TO_LAUNCH);
    m_ui->comboBox_action->addItem(tr("NAV: Land"),MAV_CMD_NAV_LAND);
//    m_ui->comboBox_action->addItem(tr("NAV: Target"),MAV_CMD_NAV_TARGET);
    //m_ui->comboBox_action->addItem(tr("IF: Delay over"),MAV_CMD_CONDITION_DELAY);
    //m_ui->comboBox_action->addItem(tr("IF: Yaw angle is"),MAV_CMD_CONDITION_YAW);
    //m_ui->comboBox_action->addItem(tr("DO: Jump to Index"),MAV_CMD_DO_JUMP);
    m_ui->comboBox_action->addItem(tr("Other"), MAV_CMD_ENUM_END);

    // add frames
    m_ui->comboBox_frame->addItem("Abs. Alt/Global",MAV_FRAME_GLOBAL);
    m_ui->comboBox_frame->addItem("Rel. Alt/Global", MAV_FRAME_GLOBAL_RELATIVE_ALT);
    m_ui->comboBox_frame->addItem("Local/Abs. Alt.",MAV_FRAME_LOCAL);
    m_ui->comboBox_frame->addItem("Mission",MAV_FRAME_MISSION);

    // Initialize view correctly
    updateActionView(wp->getAction());
    updateFrameView(wp->getFrame());

    // Read values and set user interface
    updateValues();

    // Check for mission frame
    if (wp->getFrame() == MAV_FRAME_MISSION) {
        m_ui->comboBox_action->setCurrentIndex(m_ui->comboBox_action->count()-1);
    }

    connect(m_ui->posNSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setX(double)));
    connect(m_ui->posESpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setY(double)));
    connect(m_ui->posDSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setZ(double)));

    connect(m_ui->latSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setLatitude(double)));
    connect(m_ui->lonSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setLongitude(double)));
    connect(m_ui->altSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setAltitude(double)));
    connect(m_ui->yawSpinBox, SIGNAL(valueChanged(int)), wp, SLOT(setYaw(int)));

    connect(m_ui->upButton, SIGNAL(clicked()), this, SLOT(moveUp()));
    connect(m_ui->downButton, SIGNAL(clicked()), this, SLOT(moveDown()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(remove()));

    connect(m_ui->autoContinue, SIGNAL(stateChanged(int)), this, SLOT(changedAutoContinue(int)));
    connect(m_ui->selectedBox, SIGNAL(stateChanged(int)), this, SLOT(changedCurrent(int)));
    connect(m_ui->comboBox_action, SIGNAL(activated(int)), this, SLOT(changedAction(int)));
    connect(m_ui->comboBox_frame, SIGNAL(activated(int)), this, SLOT(changedFrame(int)));

    connect(m_ui->orbitSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setLoiterOrbit(double)));
    connect(m_ui->acceptanceSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setAcceptanceRadius(double)));
    connect(m_ui->holdTimeSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setHoldTime(double)));
    connect(m_ui->turnsSpinBox, SIGNAL(valueChanged(int)), wp, SLOT(setTurns(int)));
    connect(m_ui->takeOffAngleSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam1(double)));

    // Connect actions
    connect(customCommand->commandSpinBox, SIGNAL(valueChanged(int)),   wp, SLOT(setAction(int)));
    connect(customCommand->param1SpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam1(double)));
    connect(customCommand->param2SpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam2(double)));
    connect(customCommand->param3SpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam3(double)));
    connect(customCommand->param4SpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam4(double)));
    connect(customCommand->param5SpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam5(double)));
    connect(customCommand->param6SpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam6(double)));
    connect(customCommand->param7SpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setParam7(double)));
}

void WaypointView::moveUp()
{
    emit moveUpWaypoint(wp);
}

void WaypointView::moveDown()
{
    emit moveDownWaypoint(wp);
}


void WaypointView::remove()
{
    emit removeWaypoint(wp);
    deleteLater();
}

void WaypointView::changedAutoContinue(int state)
{
    if (state == 0)
        wp->setAutocontinue(false);
    else
        wp->setAutocontinue(true);
}

void WaypointView::updateActionView(int action)
{
    // Remove stretch item at index 17 (m_ui->removeSpacer)
    m_ui->horizontalLayout->takeAt(17);
    // expose ui based on action

    switch(action) {
    case MAV_CMD_NAV_TAKEOFF:
        m_ui->orbitSpinBox->hide();
        m_ui->yawSpinBox->hide();
        m_ui->turnsSpinBox->hide();
        m_ui->autoContinue->hide();
        m_ui->holdTimeSpinBox->hide();
        m_ui->acceptanceSpinBox->hide();
        m_ui->customActionWidget->hide();
        m_ui->horizontalLayout->insertStretch(17, 82);
        m_ui->takeOffAngleSpinBox->show();
        break;
    case MAV_CMD_NAV_LAND:
        m_ui->orbitSpinBox->hide();
        m_ui->takeOffAngleSpinBox->hide();
        m_ui->yawSpinBox->hide();
        m_ui->turnsSpinBox->hide();
        m_ui->autoContinue->hide();
        m_ui->holdTimeSpinBox->hide();
        m_ui->acceptanceSpinBox->hide();
        m_ui->customActionWidget->hide();
        m_ui->horizontalLayout->insertStretch(17, 26);
        break;
    case MAV_CMD_NAV_RETURN_TO_LAUNCH:
        m_ui->orbitSpinBox->hide();
        m_ui->takeOffAngleSpinBox->hide();
        m_ui->yawSpinBox->hide();
        m_ui->turnsSpinBox->hide();
        m_ui->autoContinue->hide();
        m_ui->holdTimeSpinBox->hide();
        m_ui->acceptanceSpinBox->hide();
        m_ui->customActionWidget->hide();
        m_ui->horizontalLayout->insertStretch(17, 26);
        break;
    case MAV_CMD_NAV_WAYPOINT:
        m_ui->orbitSpinBox->hide();
        m_ui->takeOffAngleSpinBox->hide();
        m_ui->turnsSpinBox->hide();
        m_ui->holdTimeSpinBox->show();
        m_ui->customActionWidget->hide();
        m_ui->horizontalLayout->insertStretch(17, 1);

        m_ui->autoContinue->show();
        m_ui->acceptanceSpinBox->show();
        m_ui->yawSpinBox->show();
        break;
    case MAV_CMD_NAV_LOITER_UNLIM:
        m_ui->takeOffAngleSpinBox->hide();
        m_ui->yawSpinBox->hide();
        m_ui->turnsSpinBox->hide();
        m_ui->autoContinue->hide();
        m_ui->holdTimeSpinBox->hide();
        m_ui->acceptanceSpinBox->hide();
        m_ui->customActionWidget->hide();
        m_ui->horizontalLayout->insertStretch(17, 25);
        m_ui->orbitSpinBox->show();
        break;
    case MAV_CMD_NAV_LOITER_TURNS:
        m_ui->takeOffAngleSpinBox->hide();
        m_ui->yawSpinBox->hide();
        m_ui->autoContinue->hide();
        m_ui->holdTimeSpinBox->hide();
        m_ui->acceptanceSpinBox->hide();
        m_ui->customActionWidget->hide();
        m_ui->horizontalLayout->insertStretch(17, 20);
        m_ui->orbitSpinBox->show();
        m_ui->turnsSpinBox->show();
        break;
    case MAV_CMD_NAV_LOITER_TIME:
        m_ui->takeOffAngleSpinBox->hide();
        m_ui->yawSpinBox->hide();
        m_ui->turnsSpinBox->hide();
        m_ui->autoContinue->hide();
        m_ui->acceptanceSpinBox->hide();
        m_ui->customActionWidget->hide();
        m_ui->horizontalLayout->insertStretch(17, 20);
        m_ui->orbitSpinBox->show();
        m_ui->holdTimeSpinBox->show();
        break;
//    case MAV_CMD_NAV_ORIENTATION_TARGET:
//        m_ui->orbitSpinBox->hide();
//        m_ui->takeOffAngleSpinBox->hide();
//        m_ui->turnsSpinBox->hide();
//        m_ui->holdTimeSpinBox->show();
//        m_ui->customActionWidget->hide();

//        m_ui->autoContinue->show();
//        m_ui->acceptanceSpinBox->hide();
//        m_ui->yawSpinBox->hide();
//        break;
    default:
        break;
    }
}

/**
 * @param index The index of the combo box of the action entry, NOT the action ID
 */
void WaypointView::changedAction(int index)
{
    // set waypoint action
    int actionIndex = m_ui->comboBox_action->itemData(index).toUInt();
    if (actionIndex < MAV_CMD_ENUM_END && actionIndex >= 0) {
        MAV_CMD action = (MAV_CMD) actionIndex;
        wp->setAction(action);
    }

    // Expose ui based on action
    // Change to mission frame
    // if action is unknown

    switch(actionIndex) {
    case MAV_CMD_NAV_TAKEOFF:
    case MAV_CMD_NAV_LAND:
    case MAV_CMD_NAV_RETURN_TO_LAUNCH:
    case MAV_CMD_NAV_WAYPOINT:
    case MAV_CMD_NAV_LOITER_UNLIM:
    case MAV_CMD_NAV_LOITER_TURNS:
    case MAV_CMD_NAV_LOITER_TIME:
        changeViewMode(QGC_WAYPOINTVIEW_MODE_NAV);
        // Update frame view
        updateFrameView(m_ui->comboBox_frame->currentIndex());
        // Update view
        updateActionView(actionIndex);
        break;
    case MAV_CMD_ENUM_END:
    default:
        // Switch to mission frame
        changeViewMode(QGC_WAYPOINTVIEW_MODE_DIRECT_EDITING);
        break;
    }
}

void WaypointView::changeViewMode(QGC_WAYPOINTVIEW_MODE mode)
{
    viewMode = mode;
    switch (mode) {
    case QGC_WAYPOINTVIEW_MODE_NAV:
    case QGC_WAYPOINTVIEW_MODE_CONDITION:
        // Hide everything, show condition widget
        // TODO
    case QGC_WAYPOINTVIEW_MODE_DO:

        break;
    case QGC_WAYPOINTVIEW_MODE_DIRECT_EDITING:
        // Hide almost everything
        m_ui->orbitSpinBox->hide();
        m_ui->takeOffAngleSpinBox->hide();
        m_ui->yawSpinBox->hide();
        m_ui->turnsSpinBox->hide();
        m_ui->holdTimeSpinBox->hide();
        m_ui->acceptanceSpinBox->hide();
        m_ui->posDSpinBox->hide();
        m_ui->posESpinBox->hide();
        m_ui->posNSpinBox->hide();
        m_ui->latSpinBox->hide();
        m_ui->lonSpinBox->hide();
        m_ui->altSpinBox->hide();

        int action_index = m_ui->comboBox_action->findData(MAV_CMD_ENUM_END);
        m_ui->comboBox_action->setCurrentIndex(action_index);

        // Show action widget
        if (!m_ui->customActionWidget->isVisible()) {
            m_ui->customActionWidget->show();
        }
        if (!m_ui->autoContinue->isVisible()) {
            m_ui->autoContinue->show();
        }
        break;
    }

}

void WaypointView::updateFrameView(int frame)
{
    switch(frame) {
    case MAV_FRAME_GLOBAL:
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        m_ui->posNSpinBox->hide();
        m_ui->posESpinBox->hide();
        m_ui->posDSpinBox->hide();
        m_ui->lonSpinBox->show();
        m_ui->latSpinBox->show();
        m_ui->altSpinBox->show();
        // Coordinate frame
        m_ui->comboBox_frame->show();
        m_ui->customActionWidget->hide();
        break;
    case MAV_FRAME_LOCAL:
        m_ui->lonSpinBox->hide();
        m_ui->latSpinBox->hide();
        m_ui->altSpinBox->hide();
        m_ui->posNSpinBox->show();
        m_ui->posESpinBox->show();
        m_ui->posDSpinBox->show();
        // Coordinate frame
        m_ui->comboBox_frame->show();
        m_ui->customActionWidget->hide();
        break;
    default:
        std::cerr << "unknown frame" << std::endl;
    }
}

void WaypointView::deleted(QObject* waypoint)
{
    Q_UNUSED(waypoint);
//    if (waypoint == this->wp)
//    {
//        deleteLater();
//    }
}

void WaypointView::changedFrame(int index)
{
    // set waypoint action
    MAV_FRAME frame = (MAV_FRAME)m_ui->comboBox_frame->itemData(index).toUInt();
    wp->setFrame(frame);

    updateFrameView(frame);
}

void WaypointView::changedCurrent(int state)
{
    if (state == 0) {
        m_ui->selectedBox->setChecked(true);
        m_ui->selectedBox->setCheckState(Qt::Checked);
        wp->setCurrent(false);
    } else {
        wp->setCurrent(true);
        emit changeCurrentWaypoint(wp->getId());   //the slot changeCurrentWaypoint() in WaypointList sets all other current flags to false
    }
}

void WaypointView::updateValues()
{
    // Check if we just lost the wp, delete the widget
    // accordingly
    if (!wp) {
        deleteLater();
        return;
    }
    // Deactivate signals from the WP
    wp->blockSignals(true);
    // update frame
    MAV_FRAME frame = wp->getFrame();
    int frame_index = m_ui->comboBox_frame->findData(frame);
    if (m_ui->comboBox_frame->currentIndex() != frame_index) {
        m_ui->comboBox_frame->setCurrentIndex(frame_index);
        updateFrameView(frame);
    }
    switch(frame) {
    case MAV_FRAME_LOCAL: {
        if (m_ui->posNSpinBox->value() != wp->getX()) {
            m_ui->posNSpinBox->setValue(wp->getX());
        }
        if (m_ui->posESpinBox->value() != wp->getY()) {
            m_ui->posESpinBox->setValue(wp->getY());
        }
        if (m_ui->posDSpinBox->value() != wp->getZ()) {
            m_ui->posDSpinBox->setValue(wp->getZ());
        }
    }
    break;
    case MAV_FRAME_GLOBAL:
    case MAV_FRAME_GLOBAL_RELATIVE_ALT: {
        if (m_ui->latSpinBox->value() != wp->getLatitude()) {
            // Rounding might occur, prevent spin box from
            // firing back changes
            m_ui->latSpinBox->blockSignals(true);
            m_ui->latSpinBox->setValue(wp->getLatitude());
            m_ui->latSpinBox->blockSignals(false);
        }
        if (m_ui->lonSpinBox->value() != wp->getLongitude()) {
            // Rounding might occur, prevent spin box from
            // firing back changes
            m_ui->lonSpinBox->blockSignals(true);
            m_ui->lonSpinBox->setValue(wp->getLongitude());
            m_ui->lonSpinBox->blockSignals(false);
        }
        if (m_ui->altSpinBox->value() != wp->getAltitude()) {
            // Rounding might occur, prevent spin box from
            // firing back changes
            m_ui->altSpinBox->blockSignals(true);
            m_ui->altSpinBox->setValue(wp->getAltitude());
            m_ui->altSpinBox->blockSignals(false);
        }
    }
    break;
    default:
        // Do nothing
        break;
    }

    // Update action
    MAV_CMD action = wp->getAction();
    int action_index = m_ui->comboBox_action->findData(action);
    // Set to "Other" action if it was -1
    if (action_index == -1) {
        action_index = m_ui->comboBox_action->findData(MAV_CMD_ENUM_END);
    }
    // Only update if changed
    if (m_ui->comboBox_action->currentIndex() != action_index) {
        // If action is unknown, set direct editing mode
        if (wp->getAction() < 0 || wp->getAction() > MAV_CMD_NAV_TAKEOFF) {
            changeViewMode(QGC_WAYPOINTVIEW_MODE_DIRECT_EDITING);
        } else {
            if (viewMode != QGC_WAYPOINTVIEW_MODE_DIRECT_EDITING) {
                // Action ID known, update
                m_ui->comboBox_action->setCurrentIndex(action_index);
                updateActionView(action);
            }
        }
    }
    // Do something on actions - currently unused
//    switch(action) {
//    case MAV_CMD_NAV_TAKEOFF:
//        break;
//    case MAV_CMD_NAV_LAND:
//        break;
//    case MAV_CMD_NAV_WAYPOINT:
//        break;
//    case MAV_CMD_NAV_LOITER_UNLIM:
//        break;
//    default:
//        std::cerr << "unknown action" << std::endl;
//    }

    if (m_ui->yawSpinBox->value() != wp->getYaw()) {
        if (!m_ui->yawSpinBox->isVisible()) m_ui->yawSpinBox->blockSignals(true);
        m_ui->yawSpinBox->setValue(wp->getYaw());
        if (!m_ui->yawSpinBox->isVisible()) m_ui->yawSpinBox->blockSignals(false);
    }
    if (m_ui->selectedBox->isChecked() != wp->getCurrent()) {
        m_ui->selectedBox->setChecked(wp->getCurrent());
    }
    if (m_ui->autoContinue->isChecked() != wp->getAutoContinue()) {
        m_ui->autoContinue->setChecked(wp->getAutoContinue());
    }
    m_ui->idLabel->setText(QString("%1").arg(wp->getId()));
    if (m_ui->orbitSpinBox->value() != wp->getLoiterOrbit()) {
        if (!m_ui->orbitSpinBox->isVisible()) m_ui->orbitSpinBox->blockSignals(true);
        m_ui->orbitSpinBox->setValue(wp->getLoiterOrbit());
        if (!m_ui->orbitSpinBox->isVisible()) m_ui->orbitSpinBox->blockSignals(false);
    }
    if (m_ui->acceptanceSpinBox->value() != wp->getAcceptanceRadius()) {
        if (!m_ui->acceptanceSpinBox->isVisible()) m_ui->acceptanceSpinBox->blockSignals(true);
        m_ui->acceptanceSpinBox->setValue(wp->getAcceptanceRadius());
        if (!m_ui->acceptanceSpinBox->isVisible()) m_ui->acceptanceSpinBox->blockSignals(false);
    }
    if (m_ui->holdTimeSpinBox->value() != wp->getHoldTime()) {
        if (!m_ui->holdTimeSpinBox->isVisible()) m_ui->holdTimeSpinBox->blockSignals(true);
        m_ui->holdTimeSpinBox->setValue(wp->getHoldTime());
        if (!m_ui->holdTimeSpinBox->isVisible()) m_ui->holdTimeSpinBox->blockSignals(false);
    }
    if (m_ui->turnsSpinBox->value() != wp->getTurns()) {
        if (!m_ui->turnsSpinBox->isVisible()) m_ui->turnsSpinBox->blockSignals(true);
        m_ui->turnsSpinBox->setValue(wp->getTurns());
        if (!m_ui->turnsSpinBox->isVisible()) m_ui->turnsSpinBox->blockSignals(false);
    }
    if (m_ui->takeOffAngleSpinBox->value() != wp->getParam1()) {
        if (!m_ui->takeOffAngleSpinBox->isVisible()) m_ui->takeOffAngleSpinBox->blockSignals(true);
        m_ui->takeOffAngleSpinBox->setValue(wp->getParam1());
        if (!m_ui->takeOffAngleSpinBox->isVisible()) m_ui->takeOffAngleSpinBox->blockSignals(false);
    }

    // UPDATE CUSTOM ACTION WIDGET

    if (customCommand->commandSpinBox->value() != wp->getAction()) {
        customCommand->commandSpinBox->setValue(wp->getAction());
        // qDebug() << "Changed action";
    }
    // Param 1
    if (customCommand->param1SpinBox->value() != wp->getParam1()) {
        customCommand->param1SpinBox->setValue(wp->getParam1());
    }
    // Param 2
    if (customCommand->param2SpinBox->value() != wp->getParam2()) {
        customCommand->param2SpinBox->setValue(wp->getParam2());
    }
    // Param 3
    if (customCommand->param3SpinBox->value() != wp->getParam3()) {
        customCommand->param3SpinBox->setValue(wp->getParam3());
    }
    // Param 4
    if (customCommand->param4SpinBox->value() != wp->getParam4()) {
        customCommand->param4SpinBox->setValue(wp->getParam4());
    }
    // Param 5
    if (customCommand->param5SpinBox->value() != wp->getParam5()) {
        customCommand->param5SpinBox->setValue(wp->getParam5());
    }
    // Param 6
    if (customCommand->param6SpinBox->value() != wp->getParam6()) {
        customCommand->param6SpinBox->setValue(wp->getParam6());
    }
    // Param 7
    if (customCommand->param7SpinBox->value() != wp->getParam7()) {
        customCommand->param7SpinBox->setValue(wp->getParam7());
    }

    wp->blockSignals(false);

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

        m_ui->autoContinue->setStyleSheet(checkBoxStyle);
        m_ui->selectedBox->setStyleSheet(checkBoxStyle);
        m_ui->idLabel->setStyleSheet(labelStyle);
        m_ui->groupBox->setStyleSheet(groupBoxStyle);
        lastId = currId;
    }
}

void WaypointView::setCurrent(bool state)
{
    m_ui->selectedBox->blockSignals(true);
    m_ui->selectedBox->setChecked(state);
    m_ui->selectedBox->blockSignals(false);
}

WaypointView::~WaypointView()
{
    delete m_ui;
}

void WaypointView::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
