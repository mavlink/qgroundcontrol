// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMENUBAR_P_H
#define QMENUBAR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtWidgets/qstyleoption.h"
#include <private/qmenu_p.h> // Mac needs what in this file!
#include <qpa/qplatformmenu.h>

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(menubar);

QT_BEGIN_NAMESPACE

class QMenuBarExtension;
class QMenuBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QMenuBar)
public:
    QMenuBarPrivate() : itemsDirty(0), currentAction(nullptr), mouseDown(0),
                         closePopupMode(0), defaultPopDown(1), popupState(0), keyboardState(0), altPressed(0),
                         doChildEffects(false), platformMenuBar(nullptr)
    { }

    ~QMenuBarPrivate()
        {
            delete platformMenuBar;
        }

    void init();
    QAction *getNextAction(const qsizetype start, const qsizetype increment) const;

    //item calculations
    uint itemsDirty : 1;

    QList<int> shortcutIndexMap;
    mutable QList<QRect> actionRects;
    void calcActionRects(int max_width, int start) const;
    QRect actionRect(QAction *) const;
    void updateGeometries();

    //selection
    QPointer<QAction>currentAction;
    uint mouseDown : 1, closePopupMode : 1, defaultPopDown;
    QAction *actionAt(QPoint p) const;
    void setCurrentAction(QAction *, bool =false, bool =false);
    void popupAction(QAction *, bool);

    //active popup state
    uint popupState : 1;
    QPointer<QMenu> activeMenu;

    //keyboard mode for keyboard navigation
    void focusFirstAction();
    void setKeyboardMode(bool);
    uint keyboardState : 1, altPressed : 1;
    QPointer<QWidget> keyboardFocusWidget;

    //firing of events
    void activateAction(QAction *, QAction::ActionEvent);

    void _q_actionTriggered();
    void _q_actionHovered();
    void _q_internalShortcutActivated(int);
    void _q_updateLayout();

    //extra widgets in the menubar
    QPointer<QWidget> leftWidget, rightWidget;
    QMenuBarExtension *extension;
    bool isVisible(QAction *action);

    //menu fading/scrolling effects
    bool doChildEffects;

    QRect menuRect(bool) const;

    // reparenting
    void handleReparent();
    QList<QPointer<QWidget>> oldParents;

    QList<QAction*> hiddenActions;
    //default action
    QPointer<QAction> defaultAction;

    QBasicTimer autoReleaseTimer;
    QPlatformMenuBar *platformMenuBar;
    QPlatformMenu *getPlatformMenu(const QAction *action);
    QPlatformMenu *findInsertionPlatformMenu(const QAction *action);
    void copyActionToPlatformMenu(const QAction *e, QPlatformMenu *menu);

    inline int indexOf(QAction *act) const { return q_func()->actions().indexOf(act); }
};

QT_END_NAMESPACE

#endif // QMENUBAR_P_H
