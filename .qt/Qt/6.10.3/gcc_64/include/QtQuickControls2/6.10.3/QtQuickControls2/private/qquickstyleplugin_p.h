// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTYLEPLUGIN_P_H
#define QQUICKSTYLEPLUGIN_P_H

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

#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickControls2/qtquickcontrols2global.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QQuickTheme;
class QQuickThemeChangeObserver;

class Q_QUICKCONTROLS2_EXPORT QQuickStylePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT

public:
    explicit QQuickStylePlugin(QObject *parent = nullptr);
    ~QQuickStylePlugin();

    virtual QString name() const = 0;
    virtual void initializeTheme(QQuickTheme *theme) = 0;
    virtual void updateTheme() {}

    void registerTypes(const char *uri) override;
    void unregisterTypes() override;

private:
    QQuickTheme *createTheme(const QString &name);

    struct ObserverDeleter { void operator()(QQuickThemeChangeObserver *observer); };
    std::unique_ptr<QQuickThemeChangeObserver, ObserverDeleter> themeChangeObserver;

    Q_DISABLE_COPY(QQuickStylePlugin)
};

QT_END_NAMESPACE

#endif // QQUICKSTYLEPLUGIN_P_H
