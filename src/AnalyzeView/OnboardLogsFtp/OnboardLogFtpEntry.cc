#include "OnboardLogFtpEntry.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(OnboardLogFtpEntryLog, "AnalyzeView.QGCOnboardLogFtpEntry")

QGCOnboardLogFtpEntry::QGCOnboardLogFtpEntry(uint logId, const QDateTime &dateTime, uint logSize, bool received, QObject *parent)
    : QObject(parent)
    , _logID(logId)
    , _logSize(logSize)
    , _logTimeUTC(dateTime)
    , _received(received)
{
}

QGCOnboardLogFtpEntry::~QGCOnboardLogFtpEntry()
{
}

QString QGCOnboardLogFtpEntry::sizeStr() const
{
    return QGC::bigSizeToString(_logSize);
}
