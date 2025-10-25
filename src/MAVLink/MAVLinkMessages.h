/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCMAVLinkMessagesLog)

namespace MAVLinkMessages
{

class MAVLinkMessage
{
public:
    MAVLinkMessage() = default;
    ~MAVLinkMessage() = default;

protected:
    mavlink_message_t message;
}

class ManualControl : public MAVLinkMessage
{
public:
    ManualControl() = default;

private:
    int16_t x = INT16_MAX;
    int16_t y = INT16_MAX;
    int16_t z = INT16_MAX;
    int16_t r = INT16_MAX;

    QBitArray buttons{32};

    QBitArray extensions{8};

    int16_t s = 0;
    int16_t t = 0;

    int16_t aux1 = 0;
    int16_t aux2 = 0;
    int16_t aux3 = 0;
    int16_t aux4 = 0;
    int16_t aux5 = 0;
    int16_t aux6 = 0;

    mavlink_manual_control_t manual_control{};
};

}; // namespace MAVLinkMessages
