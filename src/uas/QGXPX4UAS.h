#ifndef QGXPX4UAS_H
#define QGXPX4UAS_H

#include "UAS.h"

class QGXPX4UAS : public UAS
{
    Q_OBJECT
    Q_INTERFACES(UASInterface)
public:
    QGXPX4UAS(MAVLinkProtocol* mavlink, int id);

public slots:
    /** @brief Receive a MAVLink message from this MAV */
    void receiveMessage(LinkInterface* link, mavlink_message_t message);

    virtual void processParamValueMsgHook(mavlink_message_t& msg, const QString& paramName,const mavlink_param_value_t& rawValue, mavlink_param_union_t& paramValue);

};

#endif // QGXPX4UAS_H
