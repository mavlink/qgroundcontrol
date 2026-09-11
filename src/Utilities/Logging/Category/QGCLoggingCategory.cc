#include "QGCLoggingCategory.h"

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace {
struct Registry
{
    QMutex mutex;
    QStringList categories;
    QPointer<QObject> context;
    std::function<void(const QString&)> observer;
};

Registry& registry()
{
    static Registry state;
    return state;
}
}  // namespace

QGCLoggingCategory::QGCLoggingCategory(const QString& category)
{
    auto& state = registry();
    QMutexLocker lock(&state.mutex);
    if (state.categories.contains(category))
        return;
    state.categories.append(category);
    if (state.context && state.observer) {
        QMetaObject::invokeMethod(
            state.context, [observer = state.observer, category] { observer(category); }, Qt::QueuedConnection);
    }
}

QStringList qgcObserveLoggingCategories(QObject* context, std::function<void(const QString&)> observer)
{
    auto& state = registry();
    QMutexLocker lock(&state.mutex);
    state.context = context;
    state.observer = std::move(observer);
    return state.categories;
}
