#include "LoggingCategoryItem.h"
#include "LoggingCategoryManager.h"
#include "QmlObjectListModel.h"

LoggingCategoryItem::LoggingCategoryItem(const QString& shortName,
                                         const QString& fullName,
                                         bool enabled,
                                         QObject* parent)
    : QObject(parent)
    , _shortName(shortName)
    , _fullName(fullName)
    , _enabled(enabled)
    , _children(std::make_unique<QmlObjectListModel>(this))
{
}

void LoggingCategoryItem::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }

    _enabled = enabled;
    emit enabledChanged();

    LoggingCategoryManager::instance()->setCategoryEnabled(_fullName, enabled);
}

void LoggingCategoryItem::setEnabledFromManager(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }

    _enabled = enabled;
    emit enabledChanged();
}

void LoggingCategoryItem::setExpanded(bool expanded)
{
    if (_expanded == expanded) {
        return;
    }

    _expanded = expanded;
    emit expandedChanged();
}

void LoggingCategoryItem::ensureChildModel()
{
    if (!_children) {
        _children = std::make_unique<QmlObjectListModel>(this);
    }
}

bool LoggingCategoryItem::isParent() const
{
    return _children && _children->count() > 0;
}
