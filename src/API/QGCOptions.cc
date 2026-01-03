#include "QGCOptions.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCFlyViewOptionsLog, "API.QGCFlyViewOptions");

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

QGC_LOGGING_CATEGORY(QGCOptionsLog, "API.QGCOptions");

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
