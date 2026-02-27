#include "QmlComponentInfo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QmlComponentInfoLog, "API.QmlComponentInfo");

QmlComponentInfo::QmlComponentInfo(const QString &title, QUrl url, QUrl icon, QObject *parent)
    : QObject(parent)
    , _title(title)
    , _url(url)
    , _icon(icon)
{
    // qCDebug(QmlComponentInfoLog) << Q_FUNC_INFO << this;
}

QmlComponentInfo::~QmlComponentInfo()
{
    // qCDebug(QmlComponentInfoLog) << Q_FUNC_INFO << this;
}
