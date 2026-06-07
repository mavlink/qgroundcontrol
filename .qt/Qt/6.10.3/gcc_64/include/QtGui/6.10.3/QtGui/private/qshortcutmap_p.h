// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHORTCUTMAP_P_H
#define QSHORTCUTMAP_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qkeysequence.h"
#include "QtCore/qlist.h"
#include "QtCore/qscopedpointer.h"

QT_REQUIRE_CONFIG(shortcut);

QT_BEGIN_NAMESPACE

// To enable dump output uncomment below
//#define Dump_QShortcutMap

class QKeyEvent;
struct QShortcutEntry;
class QShortcutMapPrivate;
class QObject;

class Q_GUI_EXPORT QShortcutMap
{
    Q_DECLARE_PRIVATE(QShortcutMap)
public:
    QShortcutMap();
    ~QShortcutMap();

    typedef bool (*ContextMatcher)(QObject *object, Qt::ShortcutContext context);

    int addShortcut(QObject *owner, const QKeySequence &key, Qt::ShortcutContext context, ContextMatcher matcher);
    int removeShortcut(int id, QObject *owner, const QKeySequence &key = QKeySequence());
    int setShortcutEnabled(bool enable, int id, QObject *owner, const QKeySequence &key = QKeySequence());
    int setShortcutAutoRepeat(bool on, int id, QObject *owner, const QKeySequence &key = QKeySequence());

    QKeySequence::SequenceMatch state();

    bool tryShortcut(QKeyEvent *e);
    bool hasShortcutForKeySequence(const QKeySequence &seq) const;
    QList<QKeySequence> keySequences(bool getAll = false) const;

#ifdef Dump_QShortcutMap
    void dumpMap() const;
#endif

private:
    void resetState();
    QKeySequence::SequenceMatch nextState(QKeyEvent *e);
    void dispatchEvent(QKeyEvent *e);

    QKeySequence::SequenceMatch find(QKeyEvent *e, int ignoredModifiers = 0);
    QList<const QShortcutEntry *> matches() const;
    void createNewSequences(QKeyEvent *e, QList<QKeySequence> &ksl, int ignoredModifiers);
    void clearSequence(QList<QKeySequence> &ksl);
    int translateModifiers(Qt::KeyboardModifiers modifiers);

    QScopedPointer<QShortcutMapPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QSHORTCUTMAP_P_H
