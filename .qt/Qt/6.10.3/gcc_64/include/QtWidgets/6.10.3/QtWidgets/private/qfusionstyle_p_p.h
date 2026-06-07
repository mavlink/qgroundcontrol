// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUSIONSTYLE_P_P_H
#define QFUSIONSTYLE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qcommonstyle.h"
#include "qcommonstyle_p.h"
#include <qpa/qplatformtheme.h>
#include "private/qguiapplication_p.h"

#if QT_CONFIG(style_fusion)

QT_BEGIN_NAMESPACE

class QFusionStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QFusionStyle)

public:
    QFusionStylePrivate();

    // Used for grip handles
    static constexpr QColor lightShade = QColor(255, 255, 255, 90);
    static constexpr QColor darkShade = QColor(0, 0, 0, 60);
    static constexpr QColor topShadow = QColor(0, 0, 0, 18);
    static constexpr QColor innerContrastLine = QColor(255, 255, 255, 30);

    // On mac we want a standard blue color used when the system palette is used
    bool isMacSystemPalette(const QPalette &pal) const {
        Q_UNUSED(pal);
#if defined(Q_OS_MACOS)
        const QPalette *themePalette = QGuiApplicationPrivate::platformTheme()->palette();
        if (themePalette && themePalette->color(QPalette::Normal, QPalette::Highlight) ==
                pal.color(QPalette::Normal, QPalette::Highlight) &&
            themePalette->color(QPalette::Normal, QPalette::HighlightedText) ==
                pal.color(QPalette::Normal, QPalette::HighlightedText))
            return true;
#endif
        return false;
    }

    QColor highlight(const QPalette &pal) const {
        if (isMacSystemPalette(pal))
            return QColor(60, 140, 230);
        return pal.color(QPalette::Highlight);
    }

    QColor highlightedText(const QPalette &pal) const {
        if (isMacSystemPalette(pal))
            return Qt::white;
        return pal.color(QPalette::HighlightedText);
    }

    QColor outline(const QPalette &pal) const {
        if (isHighContrast()) {
            return pal.text().color();
        }
        if (pal.window().style() == Qt::TexturePattern)
            return QColor(0, 0, 0, 160);
        auto windowColor = pal.window().color();
        if (!windowColor.isValid())
            windowColor = QPalette().window().color();
        return windowColor.darker(140);
    }

    QColor highlightedOutline(const QPalette &pal) const {
        QColor highlightedOutline = highlight(pal).darker(125);
        if (highlightedOutline.value() > 160)
            highlightedOutline.setHsl(highlightedOutline.hue(), highlightedOutline.saturation(), 160);
        return highlightedOutline;
    }

    QColor tabFrameColor(const QPalette &pal) const {
        if (pal.window().style() == Qt::TexturePattern)
            return QColor(255, 255, 255, 8);
        return buttonColor(pal).lighter(104);
    }

    QColor buttonColor(const QPalette &pal) const {
        QColor buttonColor = pal.button().color();
        int val = qGray(buttonColor.rgb());
        buttonColor = buttonColor.lighter(100 + qMax(1, (180 - val)/6));
        buttonColor.setHsv(buttonColor.hue(), buttonColor.saturation() * 0.75, buttonColor.value());
        return buttonColor;
    }

    enum {
        menuItemHMargin      =  3, // menu item hor text margin
        menuArrowHMargin     =  6, // menu arrow horizontal margin
        menuRightBorder      = 15, // right border on menus
        menuCheckMarkWidth   = 12  // checkmarks width on menus
    };

private:
    Qt::ColorScheme colorScheme() const
    {
        return QGuiApplicationPrivate::platformTheme()->colorScheme();
    }

    bool isHighContrast() const
    {
        return QGuiApplicationPrivate::platformTheme()->contrastPreference()
                == Qt::ContrastPreference::HighContrast;
    }
};

QT_END_NAMESPACE

#endif // style_fusion

#endif //QFUSIONSTYLE_P_P_H
