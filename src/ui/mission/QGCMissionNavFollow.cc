#include "QGCMissionNavFollow.h"
#include "ui_QGCMissionNavFollow.h"
#include "WaypointEditableView.h"
#include "WaypointList.h"
#include <QMessageBox>
#include "UAS.h"
#include <math.h>

QGCMissionNavFollow::QGCMissionNavFollow(WaypointEditableView* WEV) :
    QWidget(WEV),
    ui(new Ui::QGCMissionNavFollow)
{
    ui->setupUi(this);
    this->WEV = WEV;
	
	//Check and add systems that we could follow into combo box
	uasMgr = UASManager::instance();
	Q_ASSERT(uasMgr);
	connect(uasMgr, SIGNAL(UASCreated()), this, SLOT(changedUASList));
	connect(uasMgr, SIGNAL(UASremoved()), this, SLOT(changedUASList));
	changedUASList();
	
	//Using UI to change WP:
	connect(this->ui->SysFollowcomboBox, SIGNAL(currentIndexChanged(int)), WEV, SLOT(changedSysFollow(int)));
	connect(this->ui->radSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam3(double)));
    //connect(this->ui->yawSpinBox, SIGNAL(valueChanged(double)),WEV,SLOT(changedParam4(double)));
    connect(this->ui->relNSpinBox, SIGNAL(valueChanged(double)),this,SLOT(changedRelN(double)));//NED
	connect(this->ui->relESpinBox, SIGNAL(valueChanged(double)), this, SLOT(changedRelE(double)));
	connect(this->ui->relAltSpinBox, SIGNAL(valueChanged(double)), this, SLOT(changedRelAlt(double)));
	connect(this->ui->latSpinBox, SIGNAL(valueChanged(double)), WEV, SLOT(changedParam5(double)));
	connect(this->ui->lonSpinBox, SIGNAL(valueChanged(double)), WEV, SLOT(changedParam6(double)));
	connect(this->ui->altSpinBox, SIGNAL(valueChanged(double)), WEV, SLOT(changedParam7(double)));
	
    //Reading WP to update UI:
    connect(WEV,SIGNAL(frameBroadcast(MAV_FRAME)),this,SLOT(updateFrame(MAV_FRAME)));
    connect(WEV,SIGNAL(param3Broadcast(double)),this->ui->radSpinBox,SLOT(setValue(double)));
	//connect(WEV,SIGNAL(param4Broadcast(double)),this->ui->yawSpinBox,SLOT(setValue(double)));
	connect(WEV, SIGNAL(param5Broadcast(double)), this->ui->latSpinBox, SLOT(setValue(double)));
	connect(WEV, SIGNAL(param6Broadcast(double)), this->ui->lonSpinBox, SLOT(setValue(double)));
    connect(WEV,SIGNAL(param7Broadcast(double)),this->ui->altSpinBox,SLOT(setValue(double)));

	//Select first system to follow
	changedSysFollow(0);

	m_RelN = ui->relNSpinBox->value();
	m_RelE = ui->relESpinBox->value();
	m_RelAlt = ui->relAltSpinBox->value();
	lastWPUpdate.start();
}

void QGCMissionNavFollow::updateFrame(MAV_FRAME frame)
{
    switch(frame)
    {
    case MAV_FRAME_GLOBAL:
	case MAV_FRAME_GLOBAL_RELATIVE_ALT:
		break;
	default:
		//Only allow global frame
		//QMessageBox::information(this,"Not supported", "The NAV:Follow waypoint type only supports working in the global coordinate frame. Please change to the local frame!");
		break;
    }
}

QGCMissionNavFollow::~QGCMissionNavFollow()
{
    delete ui;
}

void QGCMissionNavFollow::changedSysFollow(int newIndex)
{
	//re-connect to the global position update by the newly selected system
	QList<UASInterface*> UASList = uasMgr->getUASList();
	if (newIndex < UASList.size()) {
		connect(uasMgr->getUASForId(UASList[newIndex]->getUASID()), SIGNAL(globalPositionChanged(UASInterface*, double, double, double, double, quint64)), this, SLOT(FollowWaypointUpdate(UASInterface*, double, double, double, double, quint64)));
	}
}

void QGCMissionNavFollow::FollowWaypointUpdate(UASInterface* SenderUAS, double lat, double lon, double altAMSL, double altWGS84, quint64 usec)
{
	//TODO Transform all this into relative coordinates w.r.t. home position later on.
	
	if (lastWPUpdate.elapsed() > ui->UpdateTimedoubleSpinBox->value()*1000.0) {
		// Calculate absolute waypoint position to follow, assuming small relative offset and location far away from the earth poles.
		const double latToMeters = 111111.0;
		ui->altSpinBox->setValue(altWGS84 + m_RelAlt);
		ui->latSpinBox->setValue(lat + m_RelN / latToMeters);
		ui->lonSpinBox->setValue(lon + m_RelE / (latToMeters * cos(lat / 180.0 * M_PI)));

		//Transmit new waypoint
		uasMgr->getActiveUASWaypointManager()->writeWaypoints();
	
		//Transmit new home position
		if (ui->SetHomecheckBox->isChecked()) {
			UAS*  tempUAS = (UAS*)uasMgr->getActiveUAS();
			tempUAS->setHomePositionSilent(lat, lon, altWGS84);
		}

		lastWPUpdate.restart();
	}
}

void QGCMissionNavFollow::changedRelN(double newRelN)
{
	m_RelN = newRelN;
}
void QGCMissionNavFollow::changedRelE(double newRelE)
{
	m_RelE = newRelE;
}
void QGCMissionNavFollow::changedRelAlt(double newRelAlt)
{
	m_RelAlt = newRelAlt;
}
void QGCMissionNavFollow::changedUASList(void)
{
	QList<UASInterface*> UASList = uasMgr->getUASList();
	
	this->ui->SysFollowcomboBox->clear();
	for (int i = 0; i < UASList.size(); i++) {
		this->ui->SysFollowcomboBox->addItem(UASList[i]->getUASName());
		connect(UASList[i], SIGNAL(nameChanged(QString)), this, SLOT(changedUASList()));
	}
}
