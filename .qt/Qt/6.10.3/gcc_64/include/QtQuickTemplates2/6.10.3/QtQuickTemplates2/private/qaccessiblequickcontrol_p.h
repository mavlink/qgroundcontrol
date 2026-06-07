// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEQUICKCONTROL_H
#define QACCESSIBLEQUICKCONTROL_H

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

#include <QtQuick/private/qaccessiblequickitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickControl;

class QAccessibleQuickControl : public QAccessibleQuickItem, public QAccessibleAttributesInterface
{
#ifdef Q_OS_INTEGRITY
    // force instantiation to avoid error #2045
    struct error2045 : QList<QAccessible::Attribute> {};
#endif
public:
    QAccessibleQuickControl(QQuickControl *control);

    // QAccessibleInterface
    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleAttributesInterface
    virtual QList<QAccessible::Attribute> attributeKeys() const override;
    virtual QVariant attributeValue(QAccessible::Attribute key) const override;

private:
    QQuickControl *control() const;
};

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKCONTROL_H
