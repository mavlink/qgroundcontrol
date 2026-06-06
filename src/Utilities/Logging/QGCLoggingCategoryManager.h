#pragma once

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSet>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

class QJSEngine;
class QQmlEngine;
class LoggingCategoryFlatModel;
class LoggingCategoryTreeModel;

class QGCLoggingCategoryManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_MOC_INCLUDE("LoggingCategoryModel.h")
    Q_PROPERTY(LoggingCategoryTreeModel* treeModel READ treeCategoryModel CONSTANT)
    Q_PROPERTY(LoggingCategoryFlatModel* flatModel READ flatCategoryModel CONSTANT)
    Q_PROPERTY(QSortFilterProxyModel* filteredFlatModel READ filteredFlatModel CONSTANT)
    Q_PROPERTY(QStringList enabledCategories READ enabledCategories NOTIFY enabledCategoriesChanged)

public:
    static QGCLoggingCategoryManager* instance();
    static void init();
    static QGCLoggingCategoryManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    void registerCategory(const QString& category);

    LoggingCategoryTreeModel* treeCategoryModel() { return _treeModel; }

    LoggingCategoryFlatModel* flatCategoryModel() { return _flatModel; }

    QSortFilterProxyModel* filteredFlatModel() { return &_filteredFlatModel; }

    Q_INVOKABLE void setFilterText(const QString& text);

    Q_INVOKABLE bool isCategoryEnabled(const QString& fullCategoryName) const;
    Q_INVOKABLE void setCategoryEnabled(const QString& fullCategoryName, bool enable);
    void installFilter(const QString& commandLineLoggingOptions = QString());
    Q_INVOKABLE void disableAllCategories();
    QStringList enabledCategories() const;

signals:
    void enabledCategoriesChanged();

private:
    QGCLoggingCategoryManager();
    bool _isCategoryEnabled(const QString& fullCategoryName) const;
    void _refreshItemStates();
    static void _categoryFilter(QLoggingCategory* category);

    LoggingCategoryTreeModel* _treeModel = nullptr;
    LoggingCategoryFlatModel* _flatModel = nullptr;
    QSortFilterProxyModel _filteredFlatModel;

    mutable QReadWriteLock _filterLock;
    QSet<QString> _enabledCategories;
    QSet<QString> _commandLineCategories;
    bool _commandLineFullLogging = false;

    static QLoggingCategory::CategoryFilter s_previousFilter;
    static constexpr const char* kFilterRulesSettingsGroup = "LoggingFilters";
};
