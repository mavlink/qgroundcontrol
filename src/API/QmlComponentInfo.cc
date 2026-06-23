#include "QmlComponentInfo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QmlComponentInfoLog, "API.QmlComponentInfo");

QmlComponentInfo::QmlComponentInfo(const QString &title, QUrl url, QUrl icon, QObject *parent, bool requiresVehicle)
    : QObject(parent)
    , _title(title)
    , _url(url)
    , _icon(icon)
    , _requiresVehicle(requiresVehicle)
{
    // qCDebug(QmlComponentInfoLog) << Q_FUNC_INFO << this;
}

QmlComponentInfo::~QmlComponentInfo()
{
    // qCDebug(QmlComponentInfoLog) << Q_FUNC_INFO << this;
}
