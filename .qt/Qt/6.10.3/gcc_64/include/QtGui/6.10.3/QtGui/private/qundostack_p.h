// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUNDOSTACK_P_H
#define QUNDOSTACK_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qobject_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#if QT_CONFIG(action)
#  include <QtGui/qaction.h>
#endif

#include "qundostack.h"

QT_BEGIN_NAMESPACE
class QUndoCommand;
class QUndoGroup;

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

class QUndoCommandPrivate
{
public:
    QUndoCommandPrivate() : id(-1), obsolete(false) {}
    QList<QUndoCommand*> child_list;
    QString text;
    QString actionText;
    int id;
    bool obsolete;
};

#if QT_CONFIG(undostack)

class QUndoStackPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QUndoStack)
public:
    QUndoStackPrivate() : index(0), clean_index(0), group(nullptr), undo_limit(0) {}

    /*!
     * \internal
     * \brief Holds the presentation state for an Undo or Redo command.
     * This structure serves a change-detection purpose.
     */
    struct ActionState
    {
        bool enabled = false;
        QString text;

        friend bool operator==(const ActionState &lhs, const ActionState &rhs) noexcept
#ifdef __cpp_impl_three_way_comparison
            = default;
#else
        { return lhs.enabled == rhs.enabled && lhs.text == rhs.text; }
        friend bool operator!=(const ActionState &lhs, const ActionState &rhs) noexcept
        { return !(lhs == rhs); }
#endif
        // some compiler's reject seed = 0) = delete, overload instead:
        friend void qHash(const ActionState &key, size_t seed) = delete;
        friend void qHash(const ActionState &key) = delete;
    };

    QList<QUndoCommand*> command_list;
    QList<QUndoCommand*> macro_stack;
    int index;
    int clean_index;
    QUndoGroup *group;
    int undo_limit;
    ActionState undoActionState;
    ActionState redoActionState;

    void setIndex(int idx, bool clean);
    bool checkUndoLimit();

#ifndef QT_NO_ACTION
    static void setPrefixedText(QAction *action, const QString &prefix, const QString &defaultText, const QString &text);
#endif
};

QT_END_NAMESPACE
#endif // QT_CONFIG(undostack)
#endif // QUNDOSTACK_P_H
