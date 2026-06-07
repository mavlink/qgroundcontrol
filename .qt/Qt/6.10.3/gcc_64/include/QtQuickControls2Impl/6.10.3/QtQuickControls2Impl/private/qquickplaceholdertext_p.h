// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPLACEHOLDERTEXT_P_H
#define QQUICKPLACEHOLDERTEXT_P_H

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

#include <QtQuick/private/qquicktext_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickPlaceholderText : public QQuickText
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlaceholderText)
    QML_ADDED_IN_VERSION(2, 2)

public:
    explicit QQuickPlaceholderText(QQuickItem *parent = nullptr);

protected:
    void componentComplete() override;

    QQuickItem *textControl() const;

private Q_SLOTS:
    void updateAlignment();
};

QT_END_NAMESPACE

#endif // QQUICKPLACEHOLDERTEXT_P_H
