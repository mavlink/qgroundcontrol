// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QBUTTONGROUP_P_H
#define QBUTTONGROUP_P_H

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
#include <QtWidgets/qbuttongroup.h>

#include <QtCore/private/qobject_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qpointer.h>
#include <QtCore/qhash.h>

QT_REQUIRE_CONFIG(buttongroup);

QT_BEGIN_NAMESPACE

class QButtonGroupPrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QButtonGroup)

public:
    QButtonGroupPrivate() : exclusive(true) {}

    QList<QAbstractButton *> buttonList;
    QPointer<QAbstractButton> checkedButton;
    void detectCheckedButton();

    bool exclusive;
    QHash<QAbstractButton*, int> mapping;
};

QT_END_NAMESPACE

#endif // QBUTTONGROUP_P_H
