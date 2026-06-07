// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKIMPLICITSIZEITEM_H
#define QQUICKIMPLICITSIZEITEM_H

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

#include <private/qtquickglobal_p.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class QQuickImplicitSizeItemPrivate;
class Q_QUICK_EXPORT QQuickImplicitSizeItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 2)

protected:
    QQuickImplicitSizeItem(QQuickImplicitSizeItemPrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickImplicitSizeItem)
    Q_DECLARE_PRIVATE(QQuickImplicitSizeItem)
};

QT_END_NAMESPACE

#endif // QQUICKIMPLICITSIZEITEM_H
