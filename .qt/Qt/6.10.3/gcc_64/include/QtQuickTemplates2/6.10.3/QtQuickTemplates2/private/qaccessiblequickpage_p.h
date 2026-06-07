// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEQUICKPAGE_H
#define QACCESSIBLEQUICKPAGE_H

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

#include "qaccessiblequickcontrol_p.h"

QT_BEGIN_NAMESPACE

class QQuickPage;

class QAccessibleQuickPage : public QAccessibleQuickControl
{
public:
    QAccessibleQuickPage(QQuickPage *page);
    QAccessibleInterface *child(int index) const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
private:
    QQuickPage *page() const;
    QList<QQuickItem *> orderedChildItems() const;
};

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKPAGE_H
