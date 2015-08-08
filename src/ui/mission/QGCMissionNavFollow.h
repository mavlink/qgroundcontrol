#ifndef QGCMISSIONNAVFOLLOW_H
#define QGCMISSIONNAVFOLLOW_H

#include <QWidget>
#include "WaypointEditableView.h"
#include "UASManager.h"

namespace Ui {
    class QGCMissionNavFollow;
}

class QGCMissionNavFollow : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavFollow(WaypointEditableView* WEV);
    ~QGCMissionNavFollow();

public slots:
    void updateFrame(MAV_FRAME);
	void changedSysFollow(int);
	void changedUASList(void);
	void FollowWaypointUpdate(UASInterface* SenderUAS, double lat, double lon, double altAMSL, double altWGS84, quint64 usec);

	void changedRelN(double);
	void changedRelE(double);
	void changedRelAlt(double);

protected:
    WaypointEditableView* WEV;

	double m_RelN;
	double m_RelE;
	double m_RelAlt;

	QTime lastWPUpdate;

private:
    Ui::QGCMissionNavFollow *ui;
	UASManagerInterface* uasMgr;
};

#endif // QGCMISSIONNAVFOLLOW_H
