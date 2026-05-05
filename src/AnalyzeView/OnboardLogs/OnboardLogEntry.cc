#include "OnboardLogEntry.h"

#include "QGCFormat.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(OnboardLogEntryLog, "AnalyzeView.OnboardLogs.OnboardLogEntry")

OnboardLogEntry::OnboardLogEntry(uint logId, const QDateTime& dateTime, uint logSize, bool received, QObject* parent)
    : QObject(parent),
      _logID(logId),
      _logSize(logSize),
      _logTimeUTC(dateTime),
      _received(received),
      _state(received ? State::Available : State::Pending),
      _status(received ? tr("Available") : tr("Pending"))
{
    qCDebug(OnboardLogEntryLog) << "id" << _logID;
}

OnboardLogEntry::~OnboardLogEntry()
{
    qCDebug(OnboardLogEntryLog) << "id" << _logID;
}

QString OnboardLogEntry::sizeStr() const
{
    return QGC::bigSizeToString(_logSize);
}
