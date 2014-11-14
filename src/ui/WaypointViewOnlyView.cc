#include <QDebug>

#include "MainWindow.h"
#include "WaypointViewOnlyView.h"
#include "ui_WaypointViewOnlyView.h"

WaypointViewOnlyView::WaypointViewOnlyView(Waypoint* wp, QWidget *parent) :
    QWidget(parent),
    wp(wp),
    m_ui(new Ui::WaypointViewOnlyView)
{
    m_ui->setupUi(this);
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
    wp->setAutocontinue(new_value);
    emit changeAutoContinue(wp->getId(),new_value);
}

void WaypointViewOnlyView::changedCurrent(int state)
//This is a slot receiving signals from QCheckBox m_ui->current. The state given here is whatever the user has clicked and not the true "current" value onboard.
{
    Q_UNUSED(state);
    //qDebug() << "Trof: WaypointViewOnlyView::changedCurrent(" << state << ") ID:" << wp->getId();
    m_ui->current->blockSignals(true);

    if (m_ui->current->isChecked() == false)
    {
        if (wp->getCurrent() == true) //User clicked on the waypoint, that is already current. Box stays checked
        {
            m_ui->current->setCheckState(Qt::Checked);
            //qDebug() << "Trof: WaypointViewOnlyView::changedCurrent. Rechecked true. stay true " << m_ui->current->isChecked();
        }
        else // Strange case, unchecking the box which was not checked to start with
        {
            m_ui->current->setCheckState(Qt::Unchecked);
            //qDebug() << "Trof: WaypointViewOnlyView::changedCurrent. Unchecked false. set false " << m_ui->current->isChecked();
        }
    }
    else
    {
        m_ui->current->setCheckState(Qt::Unchecked);
        //qDebug() << "Trof: WaypointViewOnlyView::changedCurrent. Checked new. Sending set_current request to Manager " << m_ui->current->isChecked();
        emit changeCurrentWaypoint(wp->getId());   //the slot changeCurrentWaypoint() in WaypointList sets all other current flags to false

    }
    m_ui->current->blockSignals(false);
}

void WaypointViewOnlyView::setCurrent(bool state)
//This is a slot receiving signals from UASWaypointManager. The state given here is the true representation of what the "current" waypoint on UAV is.
{
    if (!wp) {
        return;
    }

    m_ui->current->blockSignals(true);
    if (state == true)
    {
        wp->setCurrent(true);
        m_ui->current->setCheckState(Qt::Checked);
    }
    else
    {
        wp->setCurrent(false);
        m_ui->current->setCheckState(Qt::Unchecked);
    }
    m_ui->current->blockSignals(false);
}

void WaypointViewOnlyView::updateValues()
{
    // Check if we just lost the wp, delete the widget
    // accordingly
    if (!wp)
    {
        deleteLater();
        return;
    }

    // Style alternating rows of Missions as lighter/darker.
    static int lastId = -1;
    int currId = wp->getId() % 2;
    if (currId != lastId)
    {
        if (currId == 1)
        {
            this->setProperty("RowColoring", "odd");
        }
        else
        {
            this->setProperty("RowColoring", "even");
        }

        lastId = currId;
    }

    // update frame

    m_ui->idLabel->setText(QString("%1").arg(wp->getId()));
    switch (wp->getFrame())
    {
    case MAV_FRAME_GLOBAL:
    {
        m_ui->frameLabel->setText(QString("GAA"));
        break;
    }
    case MAV_FRAME_LOCAL_NED:
    {
        m_ui->frameLabel->setText(QString("NED"));
        break;
    }
    case MAV_FRAME_MISSION:
    {
        m_ui->frameLabel->setText(QString("MIS"));
        break;
    }
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:
    {
        m_ui->frameLabel->setText(QString("GRA"));
        break;
    }
    case MAV_FRAME_LOCAL_ENU:
    {
        m_ui->frameLabel->setText(QString("ENU"));
        break;
    }
    default:
    {
        m_ui->frameLabel->setText(QString(""));
        break;
    }
    }

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
        switch (wp->getFrame())
        {
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        case MAV_FRAME_GLOBAL:
        {
            if (wp->getParam1()>0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and wait there for %4 sec; yaw: %5; rad: %6").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam1()).arg(wp->getParam4()).arg(wp->getParam2()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>,lon <b>%2<sup>o</sup></b>,alt <b>%3)</b>; yaw: %4; rad: %5").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam4()).arg(wp->getParam2()));
            }
            break;
        }
        case MAV_FRAME_LOCAL_NED:
        default:
        {
            if (wp->getParam1()>0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b> and wait there for %4 sec; yaw: %5; rad: %6").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f',2).arg(wp->getParam1()).arg(wp->getParam4()).arg(wp->getParam2()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b>; yaw: %4; rad: %5").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam4()).arg(wp->getParam2()));
            }
            break;
        }
        } //end Frame switch
        break;
    }
    case MAV_CMD_NAV_LOITER_UNLIM:
    {
        switch (wp->getFrame())
        {
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        case MAV_FRAME_GLOBAL:
        {
            if (wp->getParam3()>=0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and loiter there indefinitely (clockwise); rad: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam3()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and loiter there indefinitely (counter-clockwise); rad: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(-wp->getParam3()));
            }
            break;
        }
        case MAV_FRAME_LOCAL_NED:
        default:
        {
            if (wp->getParam3()>=0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b> and loiter there indefinitely (clockwise); rad: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f',2).arg(wp->getParam3()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b> and loiter there indefinitely (counter-clockwise); rad: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f', 2).arg(-wp->getParam3()));
            }
            break;
        }
        } //end Frame switch
        break;
    }
    case MAV_CMD_NAV_LOITER_TURNS:
    {
        switch (wp->getFrame())
        {
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        case MAV_FRAME_GLOBAL:
        {
            if (wp->getParam3()>=0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and loiter there for %5 turns (clockwise); rad: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam3()).arg(wp->getParam1()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and loiter there for %5 turns (counter-clockwise); rad: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(-wp->getParam3()).arg(wp->getParam1()));
            }
            break;
        }
        case MAV_FRAME_LOCAL_NED:
        default:
        {
            if (wp->getParam3()>=0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b> and loiter there for %5 turns (clockwise); rad: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f',2).arg(wp->getParam3()).arg(wp->getParam1()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b> and loiter there for %5 turns (counter-clockwise); rad: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f', 2).arg(-wp->getParam3()).arg(wp->getParam1()));
            }
            break;
        }
        } //end Frame switch
        break;
    }
    case MAV_CMD_NAV_LOITER_TIME:
    {
        switch (wp->getFrame())
        {
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        case MAV_FRAME_GLOBAL:
        {
            if (wp->getParam3()>=0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and loiter there for %5s (clockwise); rad: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam3()).arg(wp->getParam1()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and loiter there for %5s (counter-clockwise); rad: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(-wp->getParam3()).arg(wp->getParam1()));
            }
            break;
        }
        case MAV_FRAME_LOCAL_NED:
        default:
        {
            if (wp->getParam3()>=0)
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b> and loiter there for %5s (clockwise); rad: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f',2).arg(wp->getParam3()).arg(wp->getParam1()));
            }
            else
            {
                m_ui->displayBar->setText(QString("Go to <b>(%1, %2, %3)</b> and loiter there for %5s (counter-clockwise); rad: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f', 2).arg(-wp->getParam3()).arg(wp->getParam1()));
            }
            break;
        }
        } //end Frame switch
        break;
    }
    case MAV_CMD_NAV_RETURN_TO_LAUNCH:
    {
        m_ui->displayBar->setText(QString("Return to launch location"));
        break;
    }
    case MAV_CMD_NAV_LAND:
    {
        switch (wp->getFrame())
        {
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        case MAV_FRAME_GLOBAL:
        {
            m_ui->displayBar->setText(QString("LAND. Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b> and descent; yaw: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam4()));
            break;
        }
        case MAV_FRAME_LOCAL_NED:
        default:
        {
            m_ui->displayBar->setText(QString("LAND. Go to <b>(%1, %2, %3)</b> and descent; yaw: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam4()));
            break;
        }
        } //end Frame switch
        break;
    }
    case MAV_CMD_NAV_TAKEOFF:
    {
        switch (wp->getFrame())
        {
        case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        case MAV_FRAME_GLOBAL:
        {
            m_ui->displayBar->setText(QString("TAKEOFF. Go to <b>(</b>lat <b>%1<sup>o</sup></b>, lon <b>%2<sup>o</sup></b>, alt <b>%3)</b>; yaw: %4").arg(wp->getX(),0, 'f', 7).arg(wp->getY(),0, 'f', 7).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam4()));
            break;
        }
        case MAV_FRAME_LOCAL_NED:
        default:
        {
            m_ui->displayBar->setText(QString("TAKEOFF. Go to <b>(%1, %2, %3)</b>; yaw: %4").arg(wp->getX(),0, 'f', 2).arg(wp->getY(),0, 'f', 2).arg(wp->getZ(),0, 'f', 2).arg(wp->getParam4()));
            break;
        }
        } //end Frame switch
        break;
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

void WaypointViewOnlyView::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

/**
 * Implement paintEvent() so that stylesheets work for our custom widget.
 */
void WaypointViewOnlyView::paintEvent(QPaintEvent *)
 {
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
 }
