#include "QGCLoggingCategoryManager.h"

#include <QtCore/QMutex>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtQml/QJSEngine>

#include "LoggingCategoryModel.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCLoggingCategoryRegisterLog, "Utilities.QGCLoggingCategoryManager")

// QtMsgType is not severity-ordered (QtInfoMsg=4 > QtCriticalMsg=2).
static int severityRank(int qtMsgType)
{
    switch (qtMsgType) {
        case QtDebugMsg:    return 0;
        case QtInfoMsg:     return 1;
        case QtWarningMsg:  return 2;
        case QtCriticalMsg: return 3;
        case QtFatalMsg:    return 4;
        default:            return 2;
    }
}

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
        if (val.typeId() == QMetaType::Bool) {
            // Backward compat: true → Debug, false → Warning
            _categoryLevels.insert(key, val.toBool() ? QtDebugMsg : QtWarningMsg);
        } else {
            _categoryLevels.insert(key, val.toInt());
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

    int level;
    {
        QReadLocker locker(&_filterLock);
        level = _resolvedLevel(fullCategory);
    }
    auto* categoryItem = new QGCLoggingCategoryItem(shortName, fullCategory, level, this);
    _flatModel->insertSorted(categoryItem);
    _treeModel->insertCategory(segments, fullCategory, categoryItem);
}

void QGCLoggingCategoryManager::setCategoryLevel(const QString& fullCategoryName, int qtMsgLevel)
{
    qCDebug(QGCLoggingCategoryRegisterLog) << "Set category level" << fullCategoryName << qtMsgLevel;

    const bool isDefault = severityRank(qtMsgLevel) >= severityRank(kDefaultLevel);
    {
        QWriteLocker locker(&_filterLock);
        if (isDefault) {
            _categoryLevels.remove(fullCategoryName);
        } else {
            _categoryLevels.insert(fullCategoryName, qtMsgLevel);
        }
    }

    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    if (isDefault) {
        settings.remove(fullCategoryName);
    } else {
        settings.setValue(fullCategoryName, qtMsgLevel);
    }

    QLoggingCategory::installFilter(_categoryFilter);
}

void QGCLoggingCategoryManager::setCategoryEnabled(const QString& fullCategoryName, bool enable)
{
    setCategoryLevel(fullCategoryName, enable ? QtDebugMsg : kDefaultLevel);
}

bool QGCLoggingCategoryManager::isCategoryEnabled(const QString& fullCategoryName) const
{
    QReadLocker locker(&_filterLock);
    return severityRank(_resolvedLevel(fullCategoryName)) < severityRank(kDefaultLevel);
}

int QGCLoggingCategoryManager::categoryLevel(const QString& fullCategoryName) const
{
    QReadLocker locker(&_filterLock);
    return _resolvedLevel(fullCategoryName);
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

    {
        QReadLocker locker(&_filterLock);
        for (int i = 0; i < _flatModel->count(); ++i) {
            auto* item = _flatModel->at(i);
            item->setLogLevelFromManager(_resolvedLevel(item->fullCategory));
        }
    }

    s_previousFilter = QLoggingCategory::installFilter(_categoryFilter);

    qCDebug(QGCLoggingCategoryRegisterLog) << "Category filter installed";
}

int QGCLoggingCategoryManager::_resolvedLevel(const QString& fullCategoryName) const
{
    if (_commandLineFullLogging) {
        return QtDebugMsg;
    }

    const QString normalized =
        fullCategoryName.endsWith('.') ? fullCategoryName.left(fullCategoryName.size() - 1) : fullCategoryName;

    for (const QString& cmdCat : std::as_const(_commandLineCategories)) {
        if (normalized == cmdCat || normalized.startsWith(cmdCat + '.')) {
            return QtDebugMsg;
        }
    }

    // Check exact match first
    auto it = _categoryLevels.constFind(normalized);
    if (it != _categoryLevels.constEnd()) {
        return it.value();
    }

    // Check prefix matches (parent categories ending with '.')
    for (auto pit = _categoryLevels.constBegin(); pit != _categoryLevels.constEnd(); ++pit) {
        if (pit.key().endsWith('.')) {
            const QString prefix = pit.key().left(pit.key().size() - 1);
            if (normalized == prefix || normalized.startsWith(prefix + '.')) {
                return pit.value();
            }
        }
    }

    return kDefaultLevel;
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

    if (categoryName == QLatin1String("default")) {
        return;
    }

    auto* manager = instance();

    QReadLocker locker(&manager->_filterLock);
    const int rank = severityRank(manager->_resolvedLevel(categoryName));

    category->setEnabled(QtDebugMsg, rank <= 0);
    category->setEnabled(QtInfoMsg, rank <= 1);
    category->setEnabled(QtWarningMsg, rank <= 2);
    category->setEnabled(QtCriticalMsg, rank <= 3);
}

void QGCLoggingCategoryManager::disableAllCategories()
{
    qCDebug(QGCLoggingCategoryRegisterLog) << "Disabling all categories";

    {
        QWriteLocker locker(&_filterLock);
        _categoryLevels.clear();
    }

    QSettings settings;
    settings.beginGroup(kFilterRulesSettingsGroup);
    settings.remove(QString());

    for (int i = 0; i < _flatModel->count(); ++i) {
        _flatModel->at(i)->setLogLevelFromManager(kDefaultLevel);
    }

    QLoggingCategory::installFilter(_categoryFilter);
}

void QGCLoggingCategoryManager::setFilterText(const QString& text)
{
    _filteredFlatModel.setFilterFixedString(text);
}

/*===========================================================================*/
// QGCLoggingCategoryItem definitions live here — they call back into the Manager,
// and moving them avoids pulling QGCLoggingCategoryManager.h into LoggingCategoryModel.h.

QGCLoggingCategoryItem::QGCLoggingCategoryItem(const QString& shortCategory_, const QString& fullCategory_,
                                               int logLevel_, QObject* parent)
    : QObject(parent), shortCategory(shortCategory_), fullCategory(fullCategory_), _logLevel(logLevel_)
{
    connect(this, &QGCLoggingCategoryItem::logLevelChanged, this, [this]() {
        if (!_updatingFromManager) {
            QGCLoggingCategoryManager::instance()->setCategoryLevel(fullCategory, _logLevel);
        }
    });
}

void QGCLoggingCategoryItem::setEnabled(bool enabled)
{
    setLogLevel(enabled ? QtDebugMsg : QtWarningMsg);
}

void QGCLoggingCategoryItem::setLogLevel(int level)
{
    if (level != _logLevel) {
        const bool wasEnabled = enabled();
        _logLevel = level;
        emit logLevelChanged();
        if (wasEnabled != enabled()) {
            emit enabledChanged();
        }
        if (!_updatingFromManager) {
            QGCLoggingCategoryManager::instance()->setCategoryLevel(fullCategory, level);
        }
    }
}

void QGCLoggingCategoryItem::setLogLevelFromManager(int level)
{
    if (level != _logLevel) {
        const bool wasEnabled = enabled();
        _updatingFromManager = true;
        _logLevel = level;
        emit logLevelChanged();
        if (wasEnabled != enabled()) {
            emit enabledChanged();
        }
        _updatingFromManager = false;
    }
}
