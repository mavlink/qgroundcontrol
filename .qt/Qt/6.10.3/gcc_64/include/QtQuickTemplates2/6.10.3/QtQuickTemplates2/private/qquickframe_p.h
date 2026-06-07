// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFRAME_P_H
#define QQUICKFRAME_P_H

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

#include <QtQuickTemplates2/private/qquickpane_p.h>

QT_BEGIN_NAMESPACE

class QQuickFramePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickFrame : public QQuickPane
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Frame)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickFrame(QQuickItem *parent = nullptr);

protected:
    QQuickFrame(QQuickFramePrivate &dd, QQuickItem *parent);

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickFrame)
    Q_DECLARE_PRIVATE(QQuickFrame)
};

QT_END_NAMESPACE

#endif // QQUICKFRAME_P_H
