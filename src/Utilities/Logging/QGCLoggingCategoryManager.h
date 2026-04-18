#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSet>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QString>
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
    int categoryLevel(const QString& fullCategoryName) const;
    Q_INVOKABLE void setCategoryLevel(const QString& fullCategoryName, int qtMsgLevel);
    Q_INVOKABLE void setCategoryEnabled(const QString& fullCategoryName, bool enable);
    void installFilter(const QString& commandLineLoggingOptions = QString());
    Q_INVOKABLE void disableAllCategories();

private:
    QGCLoggingCategoryManager();
    int _resolvedLevel(const QString& fullCategoryName) const;
    static void _categoryFilter(QLoggingCategory* category);

    LoggingCategoryTreeModel* _treeModel = nullptr;
    LoggingCategoryFlatModel* _flatModel = nullptr;
    QSortFilterProxyModel _filteredFlatModel;

    mutable QReadWriteLock _filterLock;
    QHash<QString, int> _categoryLevels;
    QSet<QString> _commandLineCategories;
    bool _commandLineFullLogging = false;

    static QLoggingCategory::CategoryFilter s_previousFilter;
    static constexpr const char* kFilterRulesSettingsGroup = "LoggingFilters";
    static constexpr int kDefaultLevel = QtWarningMsg;
};
