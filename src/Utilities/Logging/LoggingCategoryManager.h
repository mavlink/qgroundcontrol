#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>

class QmlObjectListModel;
class LoggingCategoryItem;

class LoggingCategoryManager : public QObject
{
    Q_OBJECT

public:
    explicit LoggingCategoryManager(QObject* parent = nullptr);

    static LoggingCategoryManager* instance();

    /// Registers a logging category. Called automatically by the category filter.
    void registerCategory(const QString& category);

    /// Returns hierarchical tree model for QML display.
    QmlObjectListModel* treeCategoryModel() { return _treeCategoryModel.get(); }

    /// Returns flat list model for QML display.
    QmlObjectListModel* flatCategoryModel() { return _flatCategoryModel.get(); }

    /// Returns true if logging is enabled for the specified category.
    bool isCategoryEnabled(const QString& fullCategoryName) const;

    /// Enables/disables logging for a category. Persists to settings.
    void setCategoryEnabled(const QString& fullCategoryName, bool enabled);

    /// Installs the category filter. Call once at startup after command line parsing.
    /// @param commandLineLoggingOptions Comma-separated category names, or "full" for all
    void installFilter(const QString& commandLineLoggingOptions = QString());

    /// Disables all logging categories.
    void disableAllCategories();

    /// Finds a category item by full name. Returns nullptr if not found.
    LoggingCategoryItem* findCategory(const QString& fullCategoryName) const;

signals:
    void categoryEnabledChanged(const QString& fullCategoryName, bool enabled);

private:
    void _loadEnabledCategories();
    static void _saveEnabledCategory(const QString& fullCategoryName, bool enabled);
    static void _refreshCategories();
    static void _insertSorted(QmlObjectListModel* model, LoggingCategoryItem* item);
    LoggingCategoryItem* _findOrCreateParent(const QString& fullCategoryName);
    static QStringList _splitCategoryPath(const QString& fullCategoryName);
    bool _isCategoryEnabledByRules(const QString& fullCategoryName) const;
    static void _categoryFilter(QLoggingCategory* category);

    std::unique_ptr<QmlObjectListModel> _treeCategoryModel;
    std::unique_ptr<QmlObjectListModel> _flatCategoryModel;
    QHash<QString, LoggingCategoryItem*> _categoryLookup;
    QSet<QString> _enabledCategories;
    QSet<QString> _commandLineCategories;
    bool _commandLineFullLogging = false;

    /// Protects _categoryLookup, _enabledCategories, _commandLineCategories,
    /// _commandLineFullLogging for reads from _categoryFilter (called on any thread).
    mutable QReadWriteLock _filterLock;

    static QLoggingCategory::CategoryFilter s_previousFilter;
    static constexpr const char* kFilterRulesSettingsGroup = "LoggingFilters";
};
