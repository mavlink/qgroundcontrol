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

#ifndef QDESIGNER_COMMAND2_H
#define QDESIGNER_COMMAND2_H

#include "shared_global_p.h"
#include "qdesigner_formwindowcommand_p.h"

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class LayoutCommand;
class BreakLayoutCommand;

/* This command changes the type of a managed layout on a widget (including
 * red layouts of type 'QLayoutWidget') into another type, maintaining the
 * applicable properties. It does this by chaining BreakLayoutCommand and
 * LayoutCommand, parametrizing them not to actually delete/reparent
 * QLayoutWidget's. */

class QDESIGNER_SHARED_EXPORT MorphLayoutCommand : public QDesignerFormWindowCommand {
    Q_DISABLE_COPY_MOVE(MorphLayoutCommand)
public:
    explicit MorphLayoutCommand(QDesignerFormWindowInterface *formWindow);
    ~MorphLayoutCommand() override;

    bool init(QWidget *w, int newType);

    static bool canMorph(const QDesignerFormWindowInterface *formWindow, QWidget *w, int *ptrToCurrentType = nullptr);

    void redo() override;
    void undo() override;

private:
    static QString formatDescription(QDesignerFormEditorInterface *core, const QWidget *w, int oldType, int newType);

    BreakLayoutCommand *m_breakLayoutCommand;
    LayoutCommand *m_layoutCommand;
    int m_newType;
    QWidgetList m_widgets;
    QWidget *m_layoutBase;
};

// Change the alignment of a widget in a managed grid/box layout cell.
class LayoutAlignmentCommand : public QDesignerFormWindowCommand {
    Q_DISABLE_COPY_MOVE(LayoutAlignmentCommand)
public:
    explicit LayoutAlignmentCommand(QDesignerFormWindowInterface *formWindow);

    bool init(QWidget *w, Qt::Alignment alignment);

    void redo() override;
    void undo() override;

    // Find out alignment and return whether command is enabled.
    static Qt::Alignment alignmentOf(const QDesignerFormEditorInterface *core, QWidget *w, bool *enabled = nullptr);

private:
    static void applyAlignment(const QDesignerFormEditorInterface *core, QWidget *w, Qt::Alignment a);

    Qt::Alignment m_newAlignment;
    Qt::Alignment m_oldAlignment;
    QWidget *m_widget;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_COMMAND2_H
