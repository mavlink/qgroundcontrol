// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCONTENTITEM_P_H
#define QQUICKCONTENTITEM_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_EXPORT QQuickContentItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit QQuickContentItem(QQuickItem *parent = nullptr);
    explicit QQuickContentItem(const QObject *scope, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickContentItem)
};

QT_END_NAMESPACE

#endif // QQUICKCONTENTITEM_P_H
