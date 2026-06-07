// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKICONIMAGE_P_P_H
#define QQUICKICONIMAGE_P_P_H

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

#include <QtQuick/private/qquickimage_p_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>
#include <QtGui/private/qiconloader_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickIconImagePrivate : public QQuickImagePrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickIconImage)

    ~QQuickIconImagePrivate() override;
    void updateFillMode();
    bool updateDevicePixelRatio(qreal targetDevicePixelRatio) override;

    QUrl source;
    QColor color = Qt::transparent;
    QThemeIconInfo icon;
    bool updatingIcon = false;
    bool isThemeIcon = false;
    bool updatingFillMode = false;
};

QT_END_NAMESPACE

#endif // QQUICKICONIMAGE_P_P_H
