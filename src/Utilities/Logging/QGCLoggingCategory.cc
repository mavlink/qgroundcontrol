#include "QGCLoggingCategory.h"

#include <QtCore/QMutex>
#include <QtCore/QStringList>

#include "QGCLoggingCategoryManager.h"

// Defined in QGCLoggingCategoryManager.cc — shared early-registration buffer that
// queues QGC_LOGGING_CATEGORY static ctors that run before the manager singleton.
extern QMutex& qgcLoggingEarlyMutex();
extern QStringList*& qgcLoggingEarlyPending();

QGCLoggingCategory::QGCLoggingCategory(const QString& category)
{
    auto* mgr = QGCLoggingCategoryManager::instance();
    if (mgr) {
        mgr->registerCategory(category);
    } else {
        QMutexLocker locker(&qgcLoggingEarlyMutex());
        if (qgcLoggingEarlyPending()) {
            qgcLoggingEarlyPending()->append(category);
        }
    }
}
