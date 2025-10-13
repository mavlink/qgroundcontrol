/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkMessages.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MAVLinkMessagesLog, "MAVLink.MAVLinkMessages")

namespace MAVLinkMessages
{

void ManualControl::sendManualControlMsg(int16_t x, int16_t y, int16_t z, int16_t r,
                                         int16_t s, int16_t t,
                                         int16_t aux1, int16_t aux2, int16_t aux3, int16_t aux4, int16_t aux5, int16_t aux6,
                                         QBitArray buttons)
{
    mavlink_manual_control_t manual_control{};
    manual_control.x = x;
    manual_control.y = y;
    manual_control.z = z;
    manual_control.r = r;

    manual_control.s = s;
    manual_control.t = t;
    manual_control.aux1 = aux1;
    manual_control.aux2 = aux2;
    manual_control.aux3 = aux3;
    manual_control.aux4 = aux4;
    manual_control.aux5 = aux5;
    manual_control.aux6 = aux6;

    bool ok;
    const quint32 buttonPressed = buttons.toUInt32(QSysInfo::Endian::LittleEndian, &ok);
    if (ok) {
        manual_control.buttons = static_cast<uint16_t>(buttonPressed);
        manual_control.buttons2 = static_cast<uint16_t>((buttonPressed >> 16) & 0xFFFF);
    }
}

} // namespace MAVLinkMessages
