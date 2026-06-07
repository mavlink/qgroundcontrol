// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTFORMWINDOWMANAGER_H
#define ABSTRACTFORMWINDOWMANAGER_H

#include <QtDesigner/sdk_global.h>
#include <QtDesigner/abstractformwindow.h>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerDnDItemInterface;

class QWidget;
class QPixmap;
class QAction;
class QActionGroup;

class QDESIGNER_SDK_EXPORT QDesignerFormWindowManagerInterface: public QObject
{
    Q_OBJECT
public:
    explicit QDesignerFormWindowManagerInterface(QObject *parent = nullptr);
    virtual ~QDesignerFormWindowManagerInterface();

    enum Action
    {
#if QT_CONFIG(clipboard)
        CutAction = 100,
        CopyAction,
        PasteAction,
#endif
        DeleteAction = 103,
        SelectAllAction,

        LowerAction = 200,
        RaiseAction,

        UndoAction = 300,
        RedoAction,

        HorizontalLayoutAction = 400,
        VerticalLayoutAction,
        SplitHorizontalAction,
        SplitVerticalAction,
        GridLayoutAction,
        FormLayoutAction,
        BreakLayoutAction,
        AdjustSizeAction,
        SimplifyLayoutAction,

        DefaultPreviewAction = 500,

        FormWindowSettingsDialogAction = 600
    };
    Q_ENUM(Action)

    enum ActionGroup
    {
        StyledPreviewActionGroup = 100
    };
    Q_ENUM(ActionGroup)

    virtual QAction *action(Action action) const = 0;
    virtual QActionGroup *actionGroup(ActionGroup actionGroup) const = 0;

#if QT_CONFIG(clipboard)
    QAction *actionCut() const;
    QAction *actionCopy() const;
    QAction *actionPaste() const;
#endif
    QAction *actionDelete() const;
    QAction *actionSelectAll() const;
    QAction *actionLower() const;
    QAction *actionRaise() const;
    QAction *actionUndo() const;
    QAction *actionRedo() const;

    QAction *actionHorizontalLayout() const;
    QAction *actionVerticalLayout() const;
    QAction *actionSplitHorizontal() const;
    QAction *actionSplitVertical() const;
    QAction *actionGridLayout() const;
    QAction *actionFormLayout() const;
    QAction *actionBreakLayout() const;
    QAction *actionAdjustSize() const;
    QAction *actionSimplifyLayout() const;

    virtual QDesignerFormWindowInterface *activeFormWindow() const = 0;

    virtual int formWindowCount() const = 0;
    virtual QDesignerFormWindowInterface *formWindow(int index) const = 0;

    virtual QDesignerFormWindowInterface *createFormWindow(QWidget *parentWidget = nullptr, Qt::WindowFlags flags = Qt::WindowFlags()) = 0;

    virtual QDesignerFormEditorInterface *core() const = 0;

    virtual void dragItems(const QList<QDesignerDnDItemInterface*> &item_list) = 0;

    virtual QPixmap createPreviewPixmap() const = 0;

Q_SIGNALS:
    void formWindowAdded(QDesignerFormWindowInterface *formWindow);
    void formWindowRemoved(QDesignerFormWindowInterface *formWindow);
    void activeFormWindowChanged(QDesignerFormWindowInterface *formWindow);
    void formWindowSettingsChanged(QDesignerFormWindowInterface *fw);

public Q_SLOTS:
    virtual void addFormWindow(QDesignerFormWindowInterface *formWindow) = 0;
    virtual void removeFormWindow(QDesignerFormWindowInterface *formWindow) = 0;
    virtual void setActiveFormWindow(QDesignerFormWindowInterface *formWindow) = 0;
    virtual void showPreview() = 0;
    virtual void closeAllPreviews() = 0;
    virtual void showPluginDialog() = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTFORMWINDOWMANAGER_H
