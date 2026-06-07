// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPLATFORMTHEME_P_H
#define QQUICKPLATFORMTHEME_P_H

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

#include <QtCore/qobject.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickPlatformTheme : public QObject
{
    Q_OBJECT
    // This exposes the enums in QPlatformTheme to QML. We can't use QML_EXTENDED
    // because it's not possible to extend a QGadget (QPlatformTheme) with an QObject (us).
    QML_EXTENDED_NAMESPACE(QPlatformTheme)
    QML_NAMED_ELEMENT(PlatformTheme)
    QML_SINGLETON
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickPlatformTheme(QObject *parent = nullptr);

    Q_INVOKABLE QVariant themeHint(QPlatformTheme::ThemeHint themeHint) const;

    static QVariant getThemeHint(QPlatformTheme::ThemeHint themeHint);
};

QT_END_NAMESPACE

#endif // QQUICKPLATFORMTHEME_P_H
