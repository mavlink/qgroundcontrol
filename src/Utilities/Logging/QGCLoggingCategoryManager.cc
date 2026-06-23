#include "QGCLoggingCategoryManager.h"

#include <QtCore/QMutex>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtQml/QJSEngine>

#include "LoggingCategoryModel.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCLoggingCategoryRegisterLog, "Utilities.QGCLoggingCategoryManager")

QLoggingCategory::CategoryFilter QGCLoggingCategoryManager::s_previousFilter = nullptr;

static QGCLoggingCategoryManager* s_managerInstance = nullptr;

// Shared with QGCLoggingCategory.cc — registrations that land before the manager exists
// are buffered here, then replayed from the manager ctor. Owned as a heap QStringList*
// rather than a QStringList value so we can delete+null it exactly once, avoiding
// static-destructor ordering hazards.
QMutex& qgcLoggingEarlyMutex()
{
    static QMutex m;
    return m;
}

QStringList*& qgcLoggingEarlyPending()
{
    static QStringList* p = new QStringList;
    return p;
}

QGCLoggingCategoryManager* QGCLoggingCategoryManager::instance()
{
    return s_managerInstance;
}

void QGCLoggingCategoryManager::init()
{
    if (!s_managerInstance) {
        new QGCLoggingCategoryManager();
    }
}

QGCLoggingCategoryManager* QGCLoggingCategoryManager::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    init();
    QJSEngine::setObjectOwnership(s_managerInstance, QJSEngine::CppOwnership);
    return s_managerInstance;
}

QGCLoggingCategoryManager::QGCLoggingCategoryManager() : QObject()
{
    s_managerInstance = this;

    _treeModel = new LoggingCategoryTreeModel(this);
    _flatModel = new LoggingCategoryFlatModel(this);

    _filteredFlatModel.setSourceModel(_flatModel);
    _filteredFlatModel.setFilterRole(static_cast<int>(LoggingCategoryFlatModel::Roles::FullNameRole));
    _filteredFlatModel.setFilterCaseSensitivity(Qt::CaseInsensitive);

    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    for (const QString& key : settings.childKeys()) {
        const QVariant val = settings.value(key);
        if (val.toBool()) {
            _enabledCategories.insert(key);
        }
    }

    // Replay categories that registered before the manager was constructed
    {
        QMutexLocker locker(&qgcLoggingEarlyMutex());
        if (qgcLoggingEarlyPending()) {
            for (const QString& cat : std::as_const(*qgcLoggingEarlyPending())) {
                registerCategory(cat);
            }
            delete qgcLoggingEarlyPending();
            qgcLoggingEarlyPending() = nullptr;
        }
    }
}

void QGCLoggingCategoryManager::registerCategory(const QString& fullCategory)
{
    const QStringList segments = fullCategory.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if (segments.isEmpty()) {
        return;
    }

    const QString shortName = segments.last();

    bool enabled;
    {
        QReadLocker locker(&_filterLock);
        enabled = _isCategoryEnabled(fullCategory);
    }
    auto* categoryItem = new QGCLoggingCategoryItem(shortName, fullCategory, enabled, this);
    _flatModel->insertSorted(categoryItem);
    _treeModel->insertCategory(segments, fullCategory, categoryItem);
}

void QGCLoggingCategoryManager::setCategoryEnabled(const QString& fullCategoryName, bool enable)
{
    qCDebug(QGCLoggingCategoryRegisterLog) << "Set category enabled" << fullCategoryName << enable;

    {
        QWriteLocker locker(&_filterLock);
        if (enable) {
            _enabledCategories.insert(fullCategoryName);
        } else {
            _enabledCategories.remove(fullCategoryName);
        }
    }

    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    if (enable) {
        settings.setValue(fullCategoryName, true);
    } else {
        settings.remove(fullCategoryName);
    }

    QLoggingCategory::installFilter(_categoryFilter);

    _refreshItemStates();

    emit enabledCategoriesChanged();
}

void QGCLoggingCategoryManager::_refreshItemStates()
{
    QList<std::pair<QGCLoggingCategoryItem*, bool>> updates;
    QSet<QGCLoggingCategoryItem*> seen;
    {
        QReadLocker locker(&_filterLock);
        for (int i = 0; i < _flatModel->count(); ++i) {
            auto* item = _flatModel->at(i);
            seen.insert(item);
            updates.emplace_back(item, _isCategoryEnabled(item->fullCategory));
        }
        // Tree intermediate (group) nodes are not in the flat model; leaf nodes are.
        // Only enqueue tree items that were not already covered above.
        _treeModel->forEachItem([this, &updates, &seen](QGCLoggingCategoryItem* treeItem) {
            if (!seen.contains(treeItem)) {
                updates.emplace_back(treeItem, _isCategoryEnabled(treeItem->fullCategory));
            }
        });
    }
    for (auto& [item, enabled] : updates) {
        item->setEnabledFromManager(enabled);
    }
}

bool QGCLoggingCategoryManager::isCategoryEnabled(const QString& fullCategoryName) const
{
    QReadLocker locker(&_filterLock);
    return _isCategoryEnabled(fullCategoryName);
}

bool QGCLoggingCategoryManager::_isCategoryEnabled(const QString& fullCategoryName) const
{
    if (_commandLineFullLogging) {
        return true;
    }

    const QString normalized =
        fullCategoryName.endsWith('.') ? fullCategoryName.left(fullCategoryName.size() - 1) : fullCategoryName;

    for (const QString& cmdCat : std::as_const(_commandLineCategories)) {
        if (normalized == cmdCat || normalized.startsWith(cmdCat + '.')) {
            return true;
        }
    }

    if (_enabledCategories.contains(fullCategoryName)) {
        return true;
    }

    for (const QString& cat : std::as_const(_enabledCategories)) {
        if (cat.endsWith('.')) {
            const QString prefix = cat.left(cat.size() - 1);
            if (normalized == prefix || normalized.startsWith(prefix + '.')) {
                return true;
            }
        }
    }

    return false;
}

void QGCLoggingCategoryManager::installFilter(const QString& commandLineLoggingOptions)
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

    _refreshItemStates();

    s_previousFilter = QLoggingCategory::installFilter(_categoryFilter);

    qCDebug(QGCLoggingCategoryRegisterLog) << "Category filter installed";
}

void QGCLoggingCategoryManager::_categoryFilter(QLoggingCategory* category)
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

    // Leave "default" (uncategorized qDebug()) and "qml" (QML console.log/warn/error)
    // to Qt's built-in filter, which enables debug messages in Debug builds by default.
    // Without this, the QGC filter would suppress them at the default level (Warning).
    if (categoryName == QLatin1String("default") || categoryName == QLatin1String("qml")) {
        return;
    }

    auto* manager = instance();

    QReadLocker locker(&manager->_filterLock);
    const bool enabled = manager->_isCategoryEnabled(categoryName);

    category->setEnabled(QtDebugMsg, enabled);
    category->setEnabled(QtInfoMsg, enabled);
}

void QGCLoggingCategoryManager::disableAllCategories()
{
    qCDebug(QGCLoggingCategoryRegisterLog) << "Disabling all categories";

    {
        QWriteLocker locker(&_filterLock);
        _enabledCategories.clear();
    }

    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    settings.remove(QString());

    _refreshItemStates();

    QLoggingCategory::installFilter(_categoryFilter);

    emit enabledCategoriesChanged();
}

QStringList QGCLoggingCategoryManager::enabledCategories() const
{
    QReadLocker locker(&_filterLock);
    QStringList result;
    result.reserve(_enabledCategories.size());
    for (const QString& cat : _enabledCategories) {
        result.append(cat.endsWith('.') ? cat + QLatin1Char('*') : cat);
    }
    result.sort();
    return result;
}

void QGCLoggingCategoryManager::setFilterText(const QString& text)
{
    _filteredFlatModel.setFilterFixedString(text);
}

/*===========================================================================*/
// QGCLoggingCategoryItem definitions live here — they call back into the Manager,
// and moving them avoids pulling QGCLoggingCategoryManager.h into LoggingCategoryModel.h.

QGCLoggingCategoryItem::QGCLoggingCategoryItem(const QString& shortCategory_, const QString& fullCategory_,
                                               bool enabled_, QObject* parent)
    : QObject(parent), shortCategory(shortCategory_), fullCategory(fullCategory_), _enabled(enabled_)
{
    connect(this, &QGCLoggingCategoryItem::enabledChanged, this, [this]() {
        if (!_updatingFromManager) {
            QGCLoggingCategoryManager::instance()->setCategoryEnabled(fullCategory, _enabled);
        }
    });
}

void QGCLoggingCategoryItem::setEnabled(bool enabled)
{
    if (enabled != _enabled) {
        _enabled = enabled;
        emit enabledChanged();
    }
}

void QGCLoggingCategoryItem::setEnabledFromManager(bool enabled)
{
    if (enabled != _enabled) {
        _updatingFromManager = true;
        _enabled = enabled;
        emit enabledChanged();
        _updatingFromManager = false;
    }
}
