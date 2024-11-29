/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QmlComponentInfo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QmlComponentInfoLog, "qgc.api.qmlcomponentinfo");

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
