// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKUNIVERSALFOCUSRECTANGLE_P_H
#define QQUICKUNIVERSALFOCUSRECTANGLE_P_H

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

#include <QtQuick/qquickpainteditem.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickUniversalFocusRectangle : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FocusRectangle)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickUniversalFocusRectangle(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;
};

QT_END_NAMESPACE

#endif // QQUICKUNIVERSALFOCUSRECTANGLE_P_H
