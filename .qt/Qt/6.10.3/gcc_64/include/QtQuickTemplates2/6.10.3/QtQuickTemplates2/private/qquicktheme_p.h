// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTHEME_P_H
#define QQUICKTHEME_P_H

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

#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtCore/qscopedpointer.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>

QT_BEGIN_NAMESPACE

class QQuickThemePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickTheme
{
public:
    QQuickTheme();
    ~QQuickTheme();

    static QQuickTheme *instance();

    enum Scope {
        System,
        Button,
        CheckBox,
        ComboBox,
        GroupBox,
        ItemView,
        Label,
        ListView,
        Menu,
        MenuBar,
        RadioButton,
        SpinBox,
        Switch,
        TabBar,
        TextArea,
        TextField,
        ToolBar,
        ToolTip,
        Tumbler
    };

    static QFont font(Scope scope);
    static QPalette palette(Scope scope);

    void setFont(Scope scope, const QFont &font);
    void setPalette(Scope scope, const QPalette &palette);

    void setUsePlatformPalette(const bool enable) { preferPlatformTheme = enable; }
    bool usePlatformPalette() { return preferPlatformTheme; }

private:
    Q_DISABLE_COPY(QQuickTheme)
    Q_DECLARE_PRIVATE(QQuickTheme)
    QScopedPointer<QQuickThemePrivate> d_ptr;
    bool preferPlatformTheme = false;
};

QT_END_NAMESPACE

#endif // QQUICKTHEME_P_H
