#include "QGXPX4UAS.h"

QGXPX4UAS::QGXPX4UAS(MAVLinkProtocol* mavlink, int id) :
    UAS(mavlink, id)
{
}

/**
 * This function is called by MAVLink once a complete, uncorrupted (CRC check valid)
 * mavlink packet is received.
 *
 * @param link Hardware link the message came from (e.g. /dev/ttyUSB0 or UDP port).
 *             messages can be sent back to the system via this link
 * @param message MAVLink message, as received from the MAVLink protocol stack
 */
void QGXPX4UAS::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    UAS::receiveMessage(link, message);
}

void QGXPX4UAS::processParamValueMsgHook(mavlink_message_t& msg, const QString& paramName,const mavlink_param_value_t& rawValue, mavlink_param_union_t& paramValue)
{
    Q_UNUSED(rawValue);
    Q_UNUSED(paramValue);

    int compId = msg.compid;
    if (paramName == "SYS_AUTOSTART") {

        bool ok;

        int val = parameters.value(compId)->value(paramName).toInt(&ok);

        if (ok && val == 0) {
            emit misconfigurationDetected(this);
            qDebug() << "HINTING MISCONFIGURATION";
        }

        qDebug() << "SYS_AUTOSTART FOUND WITH VAL: " << val;
    }
}
