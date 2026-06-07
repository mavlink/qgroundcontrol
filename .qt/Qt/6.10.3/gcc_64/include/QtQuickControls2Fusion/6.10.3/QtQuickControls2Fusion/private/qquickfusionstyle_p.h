// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFUSIONSTYLE_P_H
#define QQUICKFUSIONSTYLE_P_H

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
#include <QtGui/qcolor.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2Fusion/qtquickcontrols2fusionexports.h>

QT_BEGIN_NAMESPACE

class QQuickPalette;

class Q_QUICKCONTROLS2FUSION_EXPORT QQuickFusionStyle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor lightShade READ lightShade CONSTANT FINAL)
    Q_PROPERTY(QColor darkShade READ darkShade CONSTANT FINAL)
    Q_PROPERTY(QColor topShadow READ topShadow CONSTANT FINAL)
    Q_PROPERTY(QColor innerContrastLine READ innerContrastLine CONSTANT FINAL)
    Q_PROPERTY(bool highContrast READ isHighContrast NOTIFY highContrastChanged FINAL REVISION(6, 10))
    QML_NAMED_ELEMENT(Fusion)
    QML_SINGLETON
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickFusionStyle(QObject *parent = nullptr);

    static QColor lightShade();
    static QColor darkShade();
    static QColor topShadow();
    static QColor innerContrastLine();
    static bool isHighContrast();

    Q_INVOKABLE static QColor highlight(QQuickPalette *palette);
    Q_INVOKABLE static QColor highlightedText(QQuickPalette *palette);
    Q_INVOKABLE static QColor outline(QQuickPalette *palette);
    Q_INVOKABLE static QColor highlightedOutline(QQuickPalette *palette);
    Q_INVOKABLE static QColor tabFrameColor(QQuickPalette *palette);
    Q_INVOKABLE static QColor buttonColor(QQuickPalette *palette, bool highlighted = false, bool down = false, bool hovered = false);
    Q_INVOKABLE static QColor buttonOutline(QQuickPalette *palette, bool highlighted = false, bool enabled = true);
    Q_INVOKABLE static QColor gradientStart(const QColor &baseColor);
    Q_INVOKABLE static QColor gradientStop(const QColor &baseColor);
    Q_INVOKABLE static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50);
    Q_INVOKABLE static QColor grooveColor(QQuickPalette *palette);

Q_SIGNALS:
    Q_REVISION(6, 10) void highContrastChanged();
};

QT_END_NAMESPACE

#endif // QQUICKFUSIONSTYLE_P_H
