// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSHORTCUT_P_H
#define QSHORTCUT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qshortcut.h"
#include <QtGui/qkeysequence.h>

#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qobject_p.h>

#include <private/qshortcutmap_p.h>


QT_BEGIN_NAMESPACE

class QShortcutMap;

/*
    \internal
    Private data accessed through d-pointer.
*/
class Q_GUI_EXPORT QShortcutPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QShortcut)
public:
    QShortcutPrivate() = default;

    virtual QShortcutMap::ContextMatcher contextMatcher() const;
    virtual bool handleWhatsThis() { return false; }

    static bool simpleContextMatcher(QObject *object, Qt::ShortcutContext context);

    QList<QKeySequence> sc_sequences;
    QString sc_whatsthis;
    Qt::ShortcutContext sc_context = Qt::WindowShortcut;
    bool sc_enabled = true;
    bool sc_autorepeat = true;
    QList<int> sc_ids;
    void redoGrab(QShortcutMap &map);
};

QT_END_NAMESPACE

#endif // QSHORTCUT_P_H
