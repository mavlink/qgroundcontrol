// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTITEMDELEGATE_P_H
#define QABSTRACTITEMDELEGATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qabstractitemdelegate.h"
#include <private/qobject_p.h>

#include <qvariant.h>
#include <qmetatype.h>

QT_REQUIRE_CONFIG(itemviews);

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QAbstractItemDelegatePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractItemDelegate)
public:
    explicit QAbstractItemDelegatePrivate();

    bool handleEditorEvent(QObject *object, QEvent *event);
    bool tryFixup(QWidget *editor);
    QString textForRole(Qt::ItemDataRole role, const QVariant &value, const QLocale &locale, int precision = 6) const;
    void _q_commitDataAndCloseEditor(QWidget *editor);
};

QT_END_NAMESPACE

#endif // QABSTRACTITEMDELEGATE_P_H
