// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_TABWIDGET_H
#define QDESIGNER_TABWIDGET_H

#include "shared_global_p.h"
#include "qdesigner_propertysheet_p.h"
#include "qdesigner_utils_p.h"

#include <QtCore/qhash.h>
#include <QtCore/qpointer.h>
#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;
class QTabWidget;
class QTabBar;
class QMenu;
class QAction;

namespace qdesigner_internal {
    class PromotionTaskMenu;
}

class QDESIGNER_SHARED_EXPORT QTabWidgetEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit QTabWidgetEventFilter(QTabWidget *parent);
    ~QTabWidgetEventFilter();

    // Install helper on QTabWidget
    static void install(QTabWidget *tabWidget);
    static QTabWidgetEventFilter *eventFilterOf(const QTabWidget *tabWidget);
    // Convenience to add a menu on a tackedWidget
    static QMenu *addTabWidgetContextMenuActions(const QTabWidget *tabWidget, QMenu *popup);

    // Add context menu and return page submenu or 0.
    QMenu *addContextMenuActions(QMenu *popup);

    bool eventFilter(QObject *o, QEvent *e) override;

    QDesignerFormWindowInterface *formWindow() const;

private slots:
    void removeCurrentPage();
    void addPage();
    void addPageAfter();

private:
    int pageFromPosition(const QPoint &pos, QRect &rect) const;
    QTabBar *tabBar() const;

    QTabWidget *m_tabWidget;
    mutable QPointer<QTabBar> m_cachedTabBar;
    QPoint m_pressPoint;
    QWidget *m_dropIndicator = nullptr;
    int m_dragIndex = -1;
    QWidget *m_dragPage = nullptr;
    QString m_dragLabel;
    QIcon m_dragIcon;
    bool m_mousePressed = false;
    QAction *m_actionDeletePage;
    QAction *m_actionInsertPage;
    QAction *m_actionInsertPageAfter;
    qdesigner_internal::PromotionTaskMenu* m_pagePromotionTaskMenu;
};

// PropertySheet to handle the page properties
class QDESIGNER_SHARED_EXPORT QTabWidgetPropertySheet : public QDesignerPropertySheet {
public:
    explicit QTabWidgetPropertySheet(QTabWidget *object, QObject *parent = nullptr);

    void setProperty(int index, const QVariant &value) override;
    QVariant property(int index) const override;
    bool reset(int index) override;
    bool isEnabled(int index) const override;

    // Check whether the property is to be saved. Returns false for the page
    // properties (as the property sheet has no concept of 'stored')
    static bool checkProperty(const QString &propertyName);

private:
    enum TabWidgetProperty { PropertyCurrentTabText, PropertyCurrentTabName, PropertyCurrentTabIcon,
                             PropertyCurrentTabToolTip, PropertyCurrentTabWhatsThis, PropertyTabWidgetNone };

    static TabWidgetProperty tabWidgetPropertyFromName(const QString &name);
    QTabWidget *m_tabWidget;
    struct PageData
    {
    qdesigner_internal::PropertySheetStringValue text;
    qdesigner_internal::PropertySheetStringValue tooltip;
    qdesigner_internal::PropertySheetStringValue whatsthis;
    qdesigner_internal::PropertySheetIconValue icon;
    };
    QHash<QWidget *, PageData> m_pageToData;
};

using QTabWidgetPropertySheetFactory = QDesignerPropertySheetFactory<QTabWidget, QTabWidgetPropertySheet>;

QT_END_NAMESPACE

#endif // QDESIGNER_TABWIDGET_H
