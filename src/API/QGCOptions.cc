/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCOptions.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCFlyViewOptionsLog, "qgc.api.qgcflyviewoptions");

QGCFlyViewOptions::QGCFlyViewOptions(QGCOptions *options, QObject *parent)
    : QObject(parent)
    , _options(options)
{
    // qCDebug(QGCFlyViewOptionsLog) << Q_FUNC_INFO << this;
}

QGCFlyViewOptions::~QGCFlyViewOptions()
{
    // qCDebug(QGCFlyViewOptionsLog) << Q_FUNC_INFO << this;
}

/*===========================================================================*/

QGC_LOGGING_CATEGORY(QGCOptionsLog, "qgc.api.qgcoptions");

QGCOptions::QGCOptions(QObject *parent)
    : QObject(parent)
    , _defaultFlyViewOptions(new QGCFlyViewOptions(this))
{
    // qCDebug(QGCOptionsLog) << Q_FUNC_INFO << this;
}

QGCOptions::~QGCOptions()
{
    // qCDebug(QGCOptionsLog) << Q_FUNC_INFO << this;
}
