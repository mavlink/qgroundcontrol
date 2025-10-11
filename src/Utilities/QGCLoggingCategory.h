/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QmlObjectListModel.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QMap>

class QGCLoggingCategoryItem;

/// This is a QGC specific replacement for Q_LOGGING_CATEGORY. It will register the category name into a
/// global list. It's usage is the same as Q_LOGGING_CATEOGRY.
#define QGC_LOGGING_CATEGORY(name, categoryStr) \
    static QGCLoggingCategory qgcCategory ## name (categoryStr); \
    Q_LOGGING_CATEGORY(name, categoryStr, QtWarningMsg)

#define QGC_LOGGING_CATEGORY_ON(name, categoryStr) \
    static QGCLoggingCategory qgcCategory ## name (categoryStr); \
    Q_LOGGING_CATEGORY(name, categoryStr, QtInfoMsg)

class QGCLoggingCategoryManager : public QObject
{
    Q_GADGET

public:
    static QGCLoggingCategoryManager *instance();

    /// Registers the specified logging category to the system.
    void registerCategory(const QString &category);

    /// Returns the hierarchical list of available logging category names.
    QmlObjectListModel *treeCategoryModel() { return &_treeCategoryModel; }

    /// Returns the flat list of available logging category names.
    QmlObjectListModel *flatCategoryModel() { return &_flatCategoryModel; }

    /// Returns true if logging is turned on for the specified category.
    static bool categoryLoggingOn(const QString &fullCategroryName);

    /// Sets the logging filters rules from saved settings.
    ///     @param commandLineLogggingOptions Logging options which were specified on the command line
    void setFilterRulesFromSettings(const QString &commandLineLoggingOptions);

    void disableAllCategories();

public slots:
    /// Turns on/off logging for the specified category. State is saved in app settings.
    void setCategoryLoggingOn(const QString &fullCategoryName, bool enable);

private:
    static void _splitFullCategoryName(const QString &fullCategoryName, QString &parentCategory, QString &childCategory);
    QGCLoggingCategoryItem *_findLoggingCategory(const QString &fullCategoryName);
    void _insertSorted(QmlObjectListModel* model, QGCLoggingCategoryItem* item);

    QmlObjectListModel _treeCategoryModel;
    QmlObjectListModel _flatCategoryModel;

    static constexpr const char *kFilterRulesSettingsGroup = "LoggingFilters";
};

/*===========================================================================*/

class QGCLoggingCategory
{
public:
    QGCLoggingCategory(const QString &category) { QGCLoggingCategoryManager::instance()->registerCategory(category); }
};

class QGCLoggingCategoryItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  shortCategory   MEMBER shortCategory   CONSTANT)
    Q_PROPERTY(QString  fullCategory    MEMBER fullCategory    CONSTANT)
    Q_PROPERTY(bool     enabled         READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool     expanded        MEMBER expanded        NOTIFY expandedChanged)
    Q_PROPERTY(QmlObjectListModel* children MEMBER children CONSTANT)

public:
    QGCLoggingCategoryItem(const QString& shortCategory, const QString& fullCategory, bool enabled, QObject* parent = nullptr);

    bool enabled() const { return _enabled; }
    void setEnabled(bool enabled);

    QString shortCategory;
    QString fullCategory;
    bool _enabled = false;
    bool expanded = false;
    QmlObjectListModel* children = nullptr;

signals:
    void enabledChanged();
    void expandedChanged();
};
