#ifndef _SENSESOARMAV_H_
#define _SENSESOARMAV_H_



#include "UAS.h"
#include "mavlink.h"
#include <QTimer>


class senseSoarMAV : public UAS
{
	Q_OBJECT
	Q_INTERFACES(UASInterface)

public:
	senseSoarMAV(MAVLinkProtocol* mavlink, int id);
	~senseSoarMAV(void);
public slots:
    /** @brief Receive a MAVLink message from this MAV */
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
protected:
	float m_rotVel[3]; // Rotational velocity in the body frame
	uint8_t senseSoarState;
private:
	void quat2euler(const double *quat, double &roll, double &pitch, double &yaw); 
};

#endif // _SENSESOARMAV_H_
