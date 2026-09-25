#pragma once

#include <algorithm>

#include <QtCore/QList>
#include <QtCore/QObject>

/// Frames received with one RTCM message ID; ID 0 counts unidentified frames.
struct RTCMMessageCount
{
    Q_GADGET
    Q_PROPERTY(int messageId MEMBER messageId)
    Q_PROPERTY(quint64 count MEMBER count)

public:
    int messageId = 0;
    quint64 count = 0;

    bool operator==(const RTCMMessageCount&) const = default;
};

/// Per-ID counts from an associative container, in ascending message-ID order.
template <typename Counts>
QList<RTCMMessageCount> rtcmMessageCounts(const Counts& counts)
{
    QList<RTCMMessageCount> result;
    result.reserve(counts.size());
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        result.append({it.key(), static_cast<quint64>(it.value())});
    }
    std::ranges::sort(result, {}, &RTCMMessageCount::messageId);
    return result;
}
