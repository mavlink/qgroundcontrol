#pragma once

#include <QtCore/QByteArrayView>

#include "RTCMFramer.h"

namespace RTCM {
inline bool isValidFrame(QByteArrayView frame)
{
    return RTCMFramer::isValidFrame(
        {reinterpret_cast<const uint8_t*>(frame.data()), static_cast<size_t>(frame.size())});
}
}  // namespace RTCM
