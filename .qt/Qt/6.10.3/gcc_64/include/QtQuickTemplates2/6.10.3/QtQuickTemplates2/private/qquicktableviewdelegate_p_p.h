// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTABLEVIEWDELEGATE_P_P_H
#define QQUICKTABLEVIEWDELEGATE_P_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickTableViewDelegatePrivate : public QQuickItemDelegatePrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickTableViewDelegate)

    QPalette defaultPalette() const override;

public:
    QPointer<QQuickTableView> m_tableView;
    bool current = false;
    bool selected = false;
    bool editing = false;
};

QT_END_NAMESPACE

#endif // QQUICKTABLEVIEWDELEGATE_P_P_H
