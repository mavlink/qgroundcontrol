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

#ifndef ACTIONEDITOR_H
#define ACTIONEDITOR_H

#include "shared_global_p.h"
#include "shared_enums_p.h"
#include <QtDesigner/abstractactioneditor.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QDesignerPropertyEditorInterface;
class QDesignerSettingsInterface;
class QMenu;
class QActionGroup;
class QItemSelection;
class QListWidget;
class QPushButton;
class QLineEdit;
class QToolButton;

namespace qdesigner_internal {

class ActionView;
class ResourceMimeData;

class QDESIGNER_SHARED_EXPORT ActionEditor: public QDesignerActionEditorInterface
{
    Q_OBJECT
public:
    explicit ActionEditor(QDesignerFormEditorInterface *core, QWidget *parent = nullptr,
                          Qt::WindowFlags flags = {});
    ~ActionEditor() override;

    QDesignerFormWindowInterface *formWindow() const;
    void setFormWindow(QDesignerFormWindowInterface *formWindow) override;

    QDesignerFormEditorInterface *core() const override;

    QAction *actionNew() const;
    QAction *actionDelete() const;

    QString filter() const;

    void manageAction(QAction *action) override;
    void unmanageAction(QAction *action) override;

    static ObjectNamingMode objectNamingMode() { return m_objectNamingMode; }
    static void setObjectNamingMode(ObjectNamingMode n) { m_objectNamingMode = n; }

    static QString actionTextToName(const QString &text,
                                    const QString &prefix = QLatin1StringView("action"));

    // Utility to create a configure button with menu for usage on toolbars
    static QToolButton *createConfigureMenuButton(const QString &t, QMenu **ptrToMenu);

public slots:
    void setFilter(const QString &filter);
    void mainContainerChanged();
    void clearSelection();
    void selectAction(QAction *a); // For use by the menu editor

private slots:
    void slotCurrentItemChanged(QAction *item);
    void slotSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void editAction(QAction *item, int column = -1);
    void editCurrentAction();
    void navigateToSlotCurrentAction();
    void slotActionChanged();
    void slotNewAction();
    void slotDelete();
    void resourceImageDropped(const QString &path, QAction *action);
    void slotContextMenuRequested(QContextMenuEvent *, QAction *);
    void slotViewMode(QAction *a);
    void slotSelectAssociatedWidget(QWidget *w);
#if QT_CONFIG(clipboard)
    void slotCopy();
    void slotCut();
    void slotPaste();
#endif

signals:
    void itemActivated(QAction *item, int column);
    // Context menu for item or global menu if item == 0.
    void contextMenuRequested(QMenu *menu, QAction *item);

private:
    using ActionList = QList<QAction *>;
    void deleteActions(QDesignerFormWindowInterface *formWindow, const ActionList &);
#if QT_CONFIG(clipboard)
    void copyActions(QDesignerFormWindowInterface *formWindow, const ActionList &);
#endif

    void restoreSettings();
    void saveSettings();

    void updateViewModeActions();

    static ObjectNamingMode m_objectNamingMode;

    QDesignerFormEditorInterface *m_core;
    QPointer<QDesignerFormWindowInterface> m_formWindow;
    QListWidget *m_actionGroups;

    ActionView *m_actionView;

    QAction *m_actionNew;
    QAction *m_actionEdit;
    QAction *m_actionNavigateToSlot;
#if QT_CONFIG(clipboard)
    QAction *m_actionCopy;
    QAction *m_actionCut;
    QAction *m_actionPaste;
#endif
    QAction *m_actionSelectAll;
    QAction *m_actionDelete;

    QActionGroup *m_viewModeGroup;
    QAction *m_iconViewAction;
    QAction *m_listViewAction;

    QString m_filter;
    QWidget *m_filterWidget;
    bool m_withinSelectAction = false;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // ACTIONEDITOR_H
