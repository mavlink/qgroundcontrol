#pragma once

#include <QtCore/QSet>

#include "GPSRecordingEvent.h"

/// Stateful semantic validation shared by import and export, after wire-version normalization.
class GPSRecordingValidator
{
public:
    GPSRecordingValidator();
    ~GPSRecordingValidator();
    bool check(const GPSRecordingEvent& event, QString& error);

private:
    quint64 _previous = 0;
    QSet<quint64> _sessions;
};
