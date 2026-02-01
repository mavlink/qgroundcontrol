#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QMetaObject>
#include <QtCore/QSettings>

Q_STATIC_LOGGING_CATEGORY(QGCLoggingCategoryLog, "Utilities.Logging.QGCLoggingCategory", QtWarningMsg)

Q_GLOBAL_STATIC(QGCLoggingCategoryManager, s_instance)

QLoggingCategory::CategoryFilter QGCLoggingCategoryManager::s_previousFilter = nullptr;

QGCLoggingCategoryManager* QGCLoggingCategoryManager::instance()
{
    return s_instance();
}

QGCLoggingCategoryManager::QGCLoggingCategoryManager(QObject* parent)
    : QObject(parent)
    , _treeCategoryModel(std::make_unique<QmlObjectListModel>(this))
    , _flatCategoryModel(std::make_unique<QmlObjectListModel>(this))
{
    _loadEnabledCategories();
}

void QGCLoggingCategoryManager::_loadEnabledCategories()
{
    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    for (const QString& key : settings.childKeys()) {
        if (settings.value(key, false).toBool()) {
            _enabledCategories.insert(key);
        }
    }
}

void QGCLoggingCategoryManager::_saveEnabledCategory(const QString& fullCategoryName, bool enabled)
{
    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    if (enabled) {
        settings.setValue(fullCategoryName, true);
    } else {
        settings.remove(fullCategoryName);
    }
}

QStringList QGCLoggingCategoryManager::_splitCategoryPath(const QString& fullCategoryName)
{
    return fullCategoryName.split('.', Qt::SkipEmptyParts);
}

void QGCLoggingCategoryManager::_insertSorted(QmlObjectListModel* model, QGCLoggingCategoryItem* item)
{
    const QString& itemName = item->fullName();
    for (int i = 0; i < model->count(); ++i) {
        auto* existing = qobject_cast<QGCLoggingCategoryItem*>(model->get(i));
        if (existing && itemName < existing->fullName()) {
            model->insert(i, item);
            return;
        }
    }
    model->append(item);
}

QGCLoggingCategoryItem* QGCLoggingCategoryManager::_findOrCreateParent(const QString& fullCategoryName)
{
    const QStringList parts = _splitCategoryPath(fullCategoryName);
    if (parts.size() <= 1) {
        return nullptr;
    }

    QString parentPath;
    QmlObjectListModel* currentModel = _treeCategoryModel.get();
    QGCLoggingCategoryItem* parentItem = nullptr;

    // Build hierarchy for all parent levels (e.g., "Vehicle.Comms.Serial" creates "Vehicle." and "Vehicle.Comms.")
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (!parentPath.isEmpty()) {
            parentPath += '.';
        }
        parentPath += parts[i];
        const QString parentFullName = parentPath + '.';

        auto it = _categoryLookup.find(parentFullName);
        if (it != _categoryLookup.end()) {
            parentItem = it.value();
            parentItem->ensureChildModel();
            currentModel = parentItem->children();
        } else {
            const bool enabled = _enabledCategories.contains(parentFullName);
            parentItem = new QGCLoggingCategoryItem(parts[i], parentFullName, enabled, currentModel);
            parentItem->ensureChildModel();

            _categoryLookup.insert(parentFullName, parentItem);
            _insertSorted(_flatCategoryModel.get(), parentItem);
            _insertSorted(currentModel, parentItem);

            currentModel = parentItem->children();

            qCDebug(QGCLoggingCategoryLog) << "Created parent category:" << parentFullName;
        }
    }

    return parentItem;
}

void QGCLoggingCategoryManager::registerCategory(const QString& fullCategoryName)
{
    if (_categoryLookup.contains(fullCategoryName)) {
        return;
    }

    qCDebug(QGCLoggingCategoryLog) << "Registering category:" << fullCategoryName;

    // Create parent hierarchy if needed
    QGCLoggingCategoryItem* parent = _findOrCreateParent(fullCategoryName);
    QmlObjectListModel* targetModel = parent ? parent->children() : _treeCategoryModel.get();

    // Extract short name (last component)
    const QStringList parts = _splitCategoryPath(fullCategoryName);
    const QString shortName = parts.isEmpty() ? fullCategoryName : parts.last();

    const bool enabled = _enabledCategories.contains(fullCategoryName);
    auto* item = new QGCLoggingCategoryItem(shortName, fullCategoryName, enabled, targetModel);

    _categoryLookup.insert(fullCategoryName, item);
    _insertSorted(_flatCategoryModel.get(), item);
    _insertSorted(targetModel, item);

    qCDebug(QGCLoggingCategoryLog) << "Registered category:" << fullCategoryName
                                    << "parent:" << (parent ? parent->fullName() : "root");
}

QGCLoggingCategoryItem* QGCLoggingCategoryManager::findCategory(const QString& fullCategoryName) const
{
    return _categoryLookup.value(fullCategoryName, nullptr);
}

bool QGCLoggingCategoryManager::isCategoryEnabled(const QString& fullCategoryName) const
{
    return _enabledCategories.contains(fullCategoryName);
}

void QGCLoggingCategoryManager::setCategoryEnabled(const QString& fullCategoryName, bool enabled)
{
    const bool wasEnabled = _enabledCategories.contains(fullCategoryName);
    if (wasEnabled == enabled) {
        return;
    }

    qCDebug(QGCLoggingCategoryLog) << "Setting category" << fullCategoryName << "enabled:" << enabled;

    if (enabled) {
        _enabledCategories.insert(fullCategoryName);
    } else {
        _enabledCategories.remove(fullCategoryName);
    }

    _saveEnabledCategory(fullCategoryName, enabled);

    // Update item state without triggering circular calls
    if (auto* item = findCategory(fullCategoryName)) {
        if (item->isEnabled() != enabled) {
            QSignalBlocker blocker(item);
            item->setEnabled(enabled);
        }
    }

    _refreshCategories();
    emit categoryEnabledChanged(fullCategoryName, enabled);
}

void QGCLoggingCategoryManager::installFilter(const QString& commandLineLoggingOptions)
{
    // Parse command line options
    if (!commandLineLoggingOptions.isEmpty()) {
        const QStringList categoryList = commandLineLoggingOptions.split(',', Qt::SkipEmptyParts);

        if (!categoryList.isEmpty() && categoryList.first() == QStringLiteral("full")) {
            _commandLineFullLogging = true;
        } else {
            for (const QString& category : categoryList) {
                _commandLineCategories.insert(category.trimmed());
            }
        }
    }

    // Sync UI state with cached enabled categories
    for (auto it = _categoryLookup.constBegin(); it != _categoryLookup.constEnd(); ++it) {
        QGCLoggingCategoryItem* item = it.value();
        const bool shouldBeEnabled = _enabledCategories.contains(it.key());
        if (item->isEnabled() != shouldBeEnabled) {
            QSignalBlocker blocker(item);
            item->setEnabled(shouldBeEnabled);
        }
    }

    // Install our filter, saving the previous one
    s_previousFilter = QLoggingCategory::installFilter(_categoryFilter);

    qCDebug(QGCLoggingCategoryLog) << "Category filter installed";
}

void QGCLoggingCategoryManager::_categoryFilter(QLoggingCategory* category)
{
    // Chain to previous filter first
    if (s_previousFilter) {
        s_previousFilter(category);
    }

    const QString categoryName = QString::fromLatin1(category->categoryName());

    // Skip Qt internal categories entirely (don't register, just suppress noisy ones)
    if (categoryName.startsWith(QLatin1String("qt."))) {
        if (categoryName.startsWith(QLatin1String("qt.qml.connections"))) {
            category->setEnabled(QtDebugMsg, false);
            category->setEnabled(QtWarningMsg, false);
        }
        return;
    }

    // Skip default category
    if (categoryName == QLatin1String("default")) {
        return;
    }

    QGCLoggingCategoryManager* manager = instance();

    // Auto-register if not already known (queued since we may be in static init)
    if (!manager->_categoryLookup.contains(categoryName)) {
        QMetaObject::invokeMethod(manager, [manager, categoryName]() {
            manager->registerCategory(categoryName);
        }, Qt::QueuedConnection);
    }

    // Command line "full" enables everything
    if (manager->_commandLineFullLogging) {
        category->setEnabled(QtDebugMsg, true);
        return;
    }

    // Check command line categories (exact match or prefix)
    for (const QString& cmdCat : std::as_const(manager->_commandLineCategories)) {
        if (categoryName == cmdCat || categoryName.startsWith(cmdCat + '.')) {
            category->setEnabled(QtDebugMsg, true);
            return;
        }
    }

    // Check enabled categories from settings
    for (const QString& enabledCat : std::as_const(manager->_enabledCategories)) {
        if (enabledCat.endsWith('.')) {
            // Parent category - prefix match
            const QString prefix = enabledCat.left(enabledCat.size() - 1);
            if (categoryName.startsWith(prefix)) {
                category->setEnabled(QtDebugMsg, true);
                return;
            }
        } else {
            // Exact match
            if (categoryName == enabledCat) {
                category->setEnabled(QtDebugMsg, true);
                return;
            }
        }
    }

    // Not explicitly enabled - disable debug messages (default to off)
    category->setEnabled(QtDebugMsg, false);
}

void QGCLoggingCategoryManager::_refreshCategories()
{
    // Re-trigger filter for all registered categories by reinstalling
    QLoggingCategory::installFilter(_categoryFilter);
}

void QGCLoggingCategoryManager::disableAllCategories()
{
    qCDebug(QGCLoggingCategoryLog) << "Disabling all categories";

    // Clear settings
    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    settings.remove("");

    // Clear cached state
    _enabledCategories.clear();

    // Update all items
    for (auto it = _categoryLookup.constBegin(); it != _categoryLookup.constEnd(); ++it) {
        QGCLoggingCategoryItem* item = it.value();
        if (item->isEnabled()) {
            QSignalBlocker blocker(item);
            item->setEnabled(false);
        }
    }

    _refreshCategories();
}

/*===========================================================================*/

QGCLoggingCategoryItem::QGCLoggingCategoryItem(const QString& shortName,
                                               const QString& fullName,
                                               bool enabled,
                                               QObject* parent)
    : QObject(parent)
    , _shortName(shortName)
    , _fullName(fullName)
    , _enabled(enabled)
{
}

void QGCLoggingCategoryItem::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }

    _enabled = enabled;
    emit enabledChanged();

    // Notify manager to persist and apply filter rules
    // Skip if signals are blocked (manager is already handling this change)
    if (!signalsBlocked()) {
        QGCLoggingCategoryManager::instance()->setCategoryEnabled(_fullName, enabled);
    }
}

void QGCLoggingCategoryItem::setExpanded(bool expanded)
{
    if (_expanded == expanded) {
        return;
    }

    _expanded = expanded;
    emit expandedChanged();
}

void QGCLoggingCategoryItem::ensureChildModel()
{
    if (!_children) {
        _children = std::make_unique<QmlObjectListModel>(this);
    }
}
