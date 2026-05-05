#include "OnboardLogEntry.h"

#include "QGCFormat.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(OnboardLogEntryLog, "AnalyzeView.OnboardLogEntry")

OnboardLogEntry::OnboardLogEntry(uint logId, const QDateTime& dateTime, uint logSize, bool received,
                                       QObject* parent)
    : QObject(parent), _logID(logId), _logSize(logSize), _logTimeUTC(dateTime), _received(received)
{
    qCDebug(OnboardLogEntryLog) << Q_FUNC_INFO << "id" << _logID;
}

OnboardLogEntry::~OnboardLogEntry()
{
    qCDebug(OnboardLogEntryLog) << Q_FUNC_INFO << "id" << _logID;
}

QString OnboardLogEntry::sizeStr() const
{
    return QGC::bigSizeToString(_logSize);
}
