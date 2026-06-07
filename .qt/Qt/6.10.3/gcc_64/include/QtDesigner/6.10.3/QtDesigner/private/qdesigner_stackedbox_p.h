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

#ifndef QDESIGNER_STACKEDBOX_H
#define QDESIGNER_STACKEDBOX_H

#include "shared_global_p.h"
#include "qdesigner_propertysheet_p.h"

QT_BEGIN_NAMESPACE

class QStackedWidget;
class QWidget;
class QAction;
class QMenu;
class QToolButton;

namespace qdesigner_internal {
    class PromotionTaskMenu;
}

// Event filter to be installed on a QStackedWidget in preview mode.
// Create two buttons to switch pages.

class QDESIGNER_SHARED_EXPORT QStackedWidgetPreviewEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit QStackedWidgetPreviewEventFilter(QStackedWidget *parent);

    // Install helper on QStackedWidget
    static void install(QStackedWidget *stackedWidget);
    bool eventFilter(QObject *watched, QEvent *event) override;

    void setButtonToolTipEnabled(bool v) { m_buttonToolTipEnabled = v; }
    bool buttonToolTipEnabled() const    { return m_buttonToolTipEnabled; }

public slots:
    void updateButtons();
    void prevPage();
    void nextPage();

protected:
    QStackedWidget *stackedWidget() const { return m_stackedWidget; }
    virtual void gotoPage(int page);

private:
    void updateButtonToolTip(QObject *o);

    bool m_buttonToolTipEnabled;
    QStackedWidget *m_stackedWidget;
    QToolButton *m_prev;
    QToolButton *m_next;
};

// Event filter to be installed on a QStackedWidget in editing mode.
//  In addition to the browse buttons, handles context menu and everything

class QDESIGNER_SHARED_EXPORT QStackedWidgetEventFilter : public QStackedWidgetPreviewEventFilter
{
    Q_OBJECT
public:
    explicit QStackedWidgetEventFilter(QStackedWidget *parent);

    // Install helper on QStackedWidget
    static void install(QStackedWidget *stackedWidget);
    static QStackedWidgetEventFilter *eventFilterOf(const QStackedWidget *stackedWidget);
    // Convenience to add a menu on a tackedWidget
    static QMenu *addStackedWidgetContextMenuActions(const QStackedWidget *stackedWidget, QMenu *popup);

    // Add context menu and return page submenu or 0.
    QMenu *addContextMenuActions(QMenu *popup);

private slots:
    void removeCurrentPage();
    void addPage();
    void addPageAfter();
    void changeOrder();

protected:
    void gotoPage(int page) override;

private:
    QAction *m_actionPreviousPage;
    QAction *m_actionNextPage;
    QAction *m_actionDeletePage;
    QAction *m_actionInsertPage;
    QAction *m_actionInsertPageAfter;
    QAction *m_actionChangePageOrder;
    qdesigner_internal::PromotionTaskMenu* m_pagePromotionTaskMenu;
};

// PropertySheet to handle the "currentPageName" property
class QDESIGNER_SHARED_EXPORT QStackedWidgetPropertySheet : public QDesignerPropertySheet {
public:
    explicit QStackedWidgetPropertySheet(QStackedWidget *object, QObject *parent = nullptr);

    void setProperty(int index, const QVariant &value) override;
    QVariant property(int index) const override;
    bool reset(int index) override;
    bool isEnabled(int index) const override;

    // Check whether the property is to be saved. Returns false for the page
    // properties (as the property sheet has no concept of 'stored')
    static bool checkProperty(const QString &propertyName);

private:
    QStackedWidget *m_stackedWidget;
};

using QStackedWidgetPropertySheetFactory = QDesignerPropertySheetFactory<QStackedWidget, QStackedWidgetPropertySheet>;

QT_END_NAMESPACE

#endif // QDESIGNER_STACKEDBOX_H
