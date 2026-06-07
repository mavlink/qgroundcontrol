// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ABSTRACTFORMBUILDER_H
#define ABSTRACTFORMBUILDER_H

#if 0
#  pragma qt_sync_skip_header_check
#endif

#include "uilib_global.h"

#include <QtCore/qlist.h>
#include <QtCore/qhash.h>
#include <QtCore/qdir.h>
#include <QtCore/qscopedpointer.h>

#include <QtWidgets/qsizepolicy.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE
#if 0
// pragma for syncqt, don't remove.

#pragma qt_class(QAbstractFormBuilder)
#endif

class QAction;
class QButtonGroup;
class QActionGroup;
class QComboBox;
class QIODevice;
class QIcon;
class QLayout;
class QLayoutItem;
class QListWidget;
class QObject;
class QSpacerItem;
class QTreeWidget;
class QTableWidget;
class QVariant;
class QWidget;
class QAbstractButton;
class QAbstractItemView;

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal
{
#endif

class DomAction;
class DomActionGroup;
class DomButtonGroup;
class DomButtonGroups;
class DomActionRef;
class DomBrush;
class DomColorGroup;
class DomConnections;
class DomCustomWidgets;
class DomLayout;
class DomLayoutItem;
class DomProperty;
class DomResources;
class DomSpacer;
class DomString;
class DomTabStops;
class DomUI;
class DomWidget;
class DomResourcePixmap;

class QResourceBuilder;
class QTextBuilder;
class QFormBuilderExtra;

class QDESIGNER_UILIB_EXPORT QAbstractFormBuilder
{
public:
    Q_DISABLE_COPY_MOVE(QAbstractFormBuilder)

    QAbstractFormBuilder();
    virtual ~QAbstractFormBuilder();

    QDir workingDirectory() const;
    void setWorkingDirectory(const QDir &directory);

    virtual QWidget *load(QIODevice *dev, QWidget *parentWidget = nullptr);
    virtual void save(QIODevice *dev, QWidget *widget);

    QString errorString() const;

protected:
//
// load
//
    virtual void loadExtraInfo(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget);

    virtual QWidget *create(DomUI *ui, QWidget *parentWidget);
    virtual QWidget *create(DomWidget *ui_widget, QWidget *parentWidget);
    virtual QLayout *create(DomLayout *ui_layout, QLayout *layout, QWidget *parentWidget);
    virtual QLayoutItem *create(DomLayoutItem *ui_layoutItem, QLayout *layout, QWidget *parentWidget);

    virtual QAction *create(DomAction *ui_action, QObject *parent);
    virtual QActionGroup *create(DomActionGroup *ui_action_group, QObject *parent);
    virtual void addMenuAction(QAction *action);

    virtual void applyProperties(QObject *o, const QList<DomProperty*> &properties);
    bool applyPropertyInternally(QObject *o, const QString &propertyName, const QVariant &value);

    virtual void applyTabStops(QWidget *widget, DomTabStops *tabStops);

    virtual QWidget *createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name);
    virtual QLayout *createLayout(const QString &layoutName, QObject *parent, const QString &name);
    virtual QAction *createAction(QObject *parent, const QString &name);
    virtual QActionGroup *createActionGroup(QObject *parent, const QString &name);

    virtual void createCustomWidgets(DomCustomWidgets *) {}
    virtual void createConnections(DomConnections *, QWidget *) {}
    virtual void createResources(DomResources*) {}

    virtual bool addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout);
    virtual bool addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget);

//
// save
//
    virtual void saveExtraInfo(QWidget *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);

    virtual void saveDom(DomUI *ui, QWidget *widget);

    virtual DomActionRef *createActionRefDom(QAction *action);

    virtual DomWidget *createDom(QWidget *widget, DomWidget *ui_parentWidget, bool recursive = true);
    virtual DomLayout *createDom(QLayout *layout, DomLayout *ui_layout, DomWidget *ui_parentWidget);
    virtual DomLayoutItem *createDom(QLayoutItem *item, DomLayout *ui_parentLayout, DomWidget *ui_parentWidget);
    virtual DomSpacer *createDom(QSpacerItem *spacer, DomLayout *ui_parentLayout, DomWidget *ui_parentWidget);

    virtual DomAction *createDom(QAction *action);
    virtual DomActionGroup *createDom(QActionGroup *actionGroup);
    DomButtonGroup *createDom(QButtonGroup *buttonGroup);

    virtual DomConnections *saveConnections();
    virtual DomCustomWidgets *saveCustomWidgets();
    virtual DomTabStops *saveTabStops();
    virtual DomResources *saveResources();
    DomButtonGroups *saveButtonGroups(const QWidget *mainContainer);
    virtual QList<DomProperty*> computeProperties(QObject *obj);
    virtual bool checkProperty(QObject *obj, const QString &prop) const;
    virtual DomProperty *createProperty(QObject *object, const QString &propertyName, const QVariant &value);

    virtual void layoutInfo(DomLayout *layout, QObject *parent, int *margin, int *spacing);

    void loadListWidgetExtraInfo(DomWidget *ui_widget, QListWidget *listWidget, QWidget *parentWidget);
    void loadTreeWidgetExtraInfo(DomWidget *ui_widget, QTreeWidget *treeWidget, QWidget *parentWidget);
    void loadTableWidgetExtraInfo(DomWidget *ui_widget, QTableWidget *tableWidget, QWidget *parentWidget);
    void loadComboBoxExtraInfo(DomWidget *ui_widget, QComboBox *comboBox, QWidget *parentWidget);
    void loadButtonExtraInfo(const DomWidget *ui_widget, QAbstractButton *button, QWidget *parentWidget);
    void loadItemViewExtraInfo(DomWidget *ui_widget, QAbstractItemView *itemView, QWidget *parentWidget);

    void saveListWidgetExtraInfo(QListWidget *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveTreeWidgetExtraInfo(QTreeWidget *treeWidget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveTableWidgetExtraInfo(QTableWidget *tablWidget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveComboBoxExtraInfo(QComboBox *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveButtonExtraInfo(const QAbstractButton *widget, DomWidget *ui_widget, DomWidget *ui_parentWidget);
    void saveItemViewExtraInfo(const QAbstractItemView *itemView, DomWidget *ui_widget, DomWidget *ui_parentWidget);

    void setResourceBuilder(QResourceBuilder *builder);
    QResourceBuilder *resourceBuilder() const;
    DomProperty *saveResource(const QVariant &v) const;

    void setTextBuilder(QTextBuilder *builder);
    QTextBuilder *textBuilder() const;
    DomProperty *saveText(const QString &attributeName, const QVariant &v) const;
//
// utils
//

    QVariant toVariant(const QMetaObject *meta, DomProperty *property);
    static QString toString(const DomString *str);

    using DomPropertyHash = QHash<QString, DomProperty*>;
    static DomPropertyHash propertyMap(const QList<DomProperty*> &properties);

    void setupColorGroup(QPalette &palette, QPalette::ColorGroup colorGroup, DomColorGroup *group);
    DomColorGroup *saveColorGroup(const QPalette &palette);
    QBrush setupBrush(DomBrush *brush);
    DomBrush *saveBrush(const QBrush &brush);

    void reset();
    void initialize(const DomUI *ui);

//
//  utils
//

    static QMetaEnum toolBarAreaMetaEnum();

//
//  Icon/pixmap stuff
//
    // A Pair of icon path/qrc path.
    using IconPaths = std::pair<QString, QString>;

    void setIconProperty(DomProperty &, const IconPaths &) const;
    void setPixmapProperty(DomProperty &, const IconPaths &) const;

    static const DomResourcePixmap *domPixmap(const DomProperty* p);

private:
//
//  utils
//
    static Qt::ToolBarArea toolbarAreaFromDOMAttributes(const DomPropertyHash &attributeMap);

    friend QDESIGNER_UILIB_EXPORT DomProperty *variantToDomProperty(QAbstractFormBuilder *abstractFormBuilder, const QMetaObject *meta, const QString &propertyName, const QVariant &value);
    friend QDESIGNER_UILIB_EXPORT QVariant domPropertyToVariant(QAbstractFormBuilder *abstractFormBuilder,const QMetaObject *meta, const DomProperty *property);

protected:
    QScopedPointer<QFormBuilderExtra> d;
};

#ifdef QFORMINTERNAL_NAMESPACE
}
#endif

QT_END_NAMESPACE

#endif // ABSTRACTFORMBUILDER_H
