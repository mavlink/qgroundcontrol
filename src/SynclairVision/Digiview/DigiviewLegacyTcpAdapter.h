#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "mavlink_types.h"

class DigiviewLegacyTcpAdapter final
{
public:
    enum class DecodeResult {
        Message,
        Ignored,
        Error,
    };

    [[nodiscard]] static qsizetype recordSize();

    [[nodiscard]] QByteArray encode(const mavlink_message_t& mavlinkMessage, QString& error);
    [[nodiscard]] DecodeResult decode(QByteArrayView record, mavlink_message_t& mavlinkMessage, QString& error);

private:
    bool _recordingActive = false;
    bool _recordingStateKnown = false;
};
