// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPONTHANDLER_H
#define QQUICKPONTHANDLER_H

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

#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickPointHandler : public QQuickSinglePointHandler
{
    Q_OBJECT
    Q_PROPERTY(QVector2D translation READ translation NOTIFY translationChanged)
    QML_NAMED_ELEMENT(PointHandler)
    QML_ADDED_IN_VERSION(2, 12)

public:
    explicit QQuickPointHandler(QQuickItem *parent = nullptr);

    QVector2D translation() const;

Q_SIGNALS:
    void translationChanged();

protected:
    bool wantsEventPoint(const QPointerEvent *event, const QEventPoint &point) override;
    void handleEventPoint(QPointerEvent *event, QEventPoint &point) override;
};

QT_END_NAMESPACE

#endif // QQUICKPONTHANDLER_H
