#include "LoggingCategoryManager.h"
#include "LoggingCategoryItem.h"
#include "QmlObjectListModel.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QMetaObject>
#include <QtCore/QSettings>

Q_STATIC_LOGGING_CATEGORY(LoggingCategoryManagerLog, "Utilities.Logging.LoggingCategoryManager", QtWarningMsg)

Q_GLOBAL_STATIC(LoggingCategoryManager, s_instance)

QLoggingCategory::CategoryFilter LoggingCategoryManager::s_previousFilter = nullptr;

LoggingCategoryManager* LoggingCategoryManager::instance()
{
    return s_instance();
}

LoggingCategoryManager::LoggingCategoryManager(QObject* parent)
    : QObject(parent)
    , _treeCategoryModel(std::make_unique<QmlObjectListModel>(this))
    , _flatCategoryModel(std::make_unique<QmlObjectListModel>(this))
{
    _loadEnabledCategories();
}

void LoggingCategoryManager::_loadEnabledCategories()
{
    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    for (const QString& key : settings.childKeys()) {
        if (settings.value(key, false).toBool()) {
            _enabledCategories.insert(key);
        }
    }
}

void LoggingCategoryManager::_saveEnabledCategory(const QString& fullCategoryName, bool enabled)
{
    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    if (enabled) {
        settings.setValue(fullCategoryName, true);
    } else {
        settings.remove(fullCategoryName);
    }
}

QStringList LoggingCategoryManager::_splitCategoryPath(const QString& fullCategoryName)
{
    return fullCategoryName.split('.', Qt::SkipEmptyParts);
}

void LoggingCategoryManager::_insertSorted(QmlObjectListModel* model, LoggingCategoryItem* item)
{
    const QString& itemName = item->fullName();
    for (int i = 0; i < model->count(); ++i) {
        auto* existing = qobject_cast<LoggingCategoryItem*>(model->get(i));
        if (existing && itemName < existing->fullName()) {
            model->insert(i, item);
            return;
        }
    }
    model->append(item);
}

LoggingCategoryItem* LoggingCategoryManager::_findOrCreateParent(const QString& fullCategoryName)
{
    const QStringList parts = _splitCategoryPath(fullCategoryName);
    if (parts.size() <= 1) {
        return nullptr;
    }

    QString parentPath;
    QmlObjectListModel* currentModel = _treeCategoryModel.get();
    LoggingCategoryItem* parentItem = nullptr;

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
            const bool enabled = _isCategoryEnabledByRules(parentFullName);
            parentItem = new LoggingCategoryItem(parts[i], parentFullName, enabled, currentModel);
            parentItem->ensureChildModel();

            _categoryLookup.insert(parentFullName, parentItem);
            _insertSorted(_flatCategoryModel.get(), parentItem);
            _insertSorted(currentModel, parentItem);

            currentModel = parentItem->children();

            qCDebug(LoggingCategoryManagerLog) << "Created parent category:" << parentFullName;
        }
    }

    return parentItem;
}

void LoggingCategoryManager::registerCategory(const QString& fullCategoryName)
{
    QWriteLocker locker(&_filterLock);

    if (_categoryLookup.contains(fullCategoryName)) {
        return;
    }

    qCDebug(LoggingCategoryManagerLog) << "Registering category:" << fullCategoryName;

    LoggingCategoryItem* parent = _findOrCreateParent(fullCategoryName);
    QmlObjectListModel* targetModel = parent ? parent->children() : _treeCategoryModel.get();

    const QStringList parts = _splitCategoryPath(fullCategoryName);
    const QString shortName = parts.isEmpty() ? fullCategoryName : parts.last();

    const bool enabled = _isCategoryEnabledByRules(fullCategoryName);
    auto* item = new LoggingCategoryItem(shortName, fullCategoryName, enabled, targetModel);

    _categoryLookup.insert(fullCategoryName, item);
    _insertSorted(_flatCategoryModel.get(), item);
    _insertSorted(targetModel, item);

    qCDebug(LoggingCategoryManagerLog) << "Registered category:" << fullCategoryName
                                       << "parent:" << (parent ? parent->fullName() : "root");
}

LoggingCategoryItem* LoggingCategoryManager::findCategory(const QString& fullCategoryName) const
{
    return _categoryLookup.value(fullCategoryName, nullptr);
}

bool LoggingCategoryManager::isCategoryEnabled(const QString& fullCategoryName) const
{
    return _enabledCategories.contains(fullCategoryName);
}

void LoggingCategoryManager::setCategoryEnabled(const QString& fullCategoryName, bool enabled)
{
    const bool wasEnabled = _enabledCategories.contains(fullCategoryName);
    if (wasEnabled == enabled) {
        return;
    }

    qCDebug(LoggingCategoryManagerLog) << "Setting category" << fullCategoryName << "enabled:" << enabled;

    {
        QWriteLocker locker(&_filterLock);
        if (enabled) {
            _enabledCategories.insert(fullCategoryName);
        } else {
            _enabledCategories.remove(fullCategoryName);
        }
    }

    _saveEnabledCategory(fullCategoryName, enabled);

    if (auto* item = findCategory(fullCategoryName)) {
        item->setEnabledFromManager(enabled);
    }

    _refreshCategories();
    emit categoryEnabledChanged(fullCategoryName, enabled);
}

void LoggingCategoryManager::installFilter(const QString& commandLineLoggingOptions)
{
    if (!commandLineLoggingOptions.isEmpty()) {
        const QStringList categoryList = commandLineLoggingOptions.split(',', Qt::SkipEmptyParts);

        QWriteLocker locker(&_filterLock);
        if (!categoryList.isEmpty() && categoryList.first() == QStringLiteral("full")) {
            _commandLineFullLogging = true;
        } else {
            for (const QString& category : categoryList) {
                _commandLineCategories.insert(category.trimmed());
            }
        }
    }

    for (auto it = _categoryLookup.constBegin(); it != _categoryLookup.constEnd(); ++it) {
        LoggingCategoryItem* item = it.value();
        const bool shouldBeEnabled = _isCategoryEnabledByRules(it.key());
        item->setEnabledFromManager(shouldBeEnabled);
    }

    s_previousFilter = QLoggingCategory::installFilter(_categoryFilter);

    qCDebug(LoggingCategoryManagerLog) << "Category filter installed";
}

bool LoggingCategoryManager::_isCategoryEnabledByRules(const QString& fullCategoryName) const
{
    if (_commandLineFullLogging) {
        return true;
    }

    const QString normalized = fullCategoryName.endsWith(QLatin1Char('.'))
        ? fullCategoryName.left(fullCategoryName.size() - 1)
        : fullCategoryName;

    for (const QString& cmdCat : std::as_const(_commandLineCategories)) {
        if (normalized == cmdCat || normalized.startsWith(cmdCat + QLatin1Char('.'))) {
            return true;
        }
    }

    for (const QString& enabledCat : std::as_const(_enabledCategories)) {
        if (enabledCat.endsWith(QLatin1Char('.'))) {
            const QString prefix = enabledCat.left(enabledCat.size() - 1);
            if (normalized == prefix || normalized.startsWith(prefix + QLatin1Char('.'))) {
                return true;
            }
        } else if (normalized == enabledCat) {
            return true;
        }
    }

    return false;
}

void LoggingCategoryManager::_categoryFilter(QLoggingCategory* category)
{
    if (s_previousFilter) {
        s_previousFilter(category);
    }

    const QString categoryName = QString::fromLatin1(category->categoryName());

    if (categoryName.startsWith(QLatin1String("qt."))) {
        if (categoryName.startsWith(QLatin1String("qt.qml.connections"))) {
            category->setEnabled(QtDebugMsg, false);
            category->setEnabled(QtWarningMsg, false);
        }
        return;
    }

    if (categoryName == QLatin1String("default")) {
        return;
    }

    LoggingCategoryManager* manager = instance();

    // Read lock protects _categoryLookup, _commandLineFullLogging, _commandLineCategories,
    // _enabledCategories which can be mutated from the main thread while this filter runs
    // on any thread.
    QReadLocker locker(&manager->_filterLock);

    if (!manager->_categoryLookup.contains(categoryName)) {
        // Release lock before queuing to avoid holding it across invokeMethod
        locker.unlock();
        QMetaObject::invokeMethod(manager, [manager, categoryName]() {
            manager->registerCategory(categoryName);
        }, Qt::QueuedConnection);
        locker.relock();
    }

    category->setEnabled(QtDebugMsg, manager->_isCategoryEnabledByRules(categoryName));
}

void LoggingCategoryManager::_refreshCategories()
{
    QLoggingCategory::installFilter(_categoryFilter);
}

void LoggingCategoryManager::disableAllCategories()
{
    qCDebug(LoggingCategoryManagerLog) << "Disabling all categories";

    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    settings.remove("");

    QSet<QString> previouslyEnabled;
    {
        QWriteLocker locker(&_filterLock);
        previouslyEnabled = _enabledCategories;
        _enabledCategories.clear();
    }

    for (auto it = _categoryLookup.constBegin(); it != _categoryLookup.constEnd(); ++it) {
        it.value()->setEnabledFromManager(false);
    }

    _refreshCategories();

    for (const QString& cat : std::as_const(previouslyEnabled)) {
        emit categoryEnabledChanged(cat, false);
    }
}
