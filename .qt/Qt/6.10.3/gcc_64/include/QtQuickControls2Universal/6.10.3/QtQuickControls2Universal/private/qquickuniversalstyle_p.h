// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKUNIVERSALSTYLE_P_H
#define QQUICKUNIVERSALSTYLE_P_H

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

#include <QtGui/qcolor.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2/qquickattachedpropertypropagator.h>
#include <QtQuickControls2Universal/qtquickcontrols2universalexports.h>

QT_BEGIN_NAMESPACE

class QQuickUniversalStylePrivate;

class Q_QUICKCONTROLS2UNIVERSAL_EXPORT QQuickUniversalStyle : public QQuickAttachedPropertyPropagator
{
    Q_OBJECT
    Q_PROPERTY(Theme theme READ theme WRITE setTheme RESET resetTheme NOTIFY themeChanged FINAL)
    Q_PROPERTY(QVariant accent READ accent WRITE setAccent RESET resetAccent NOTIFY accentChanged FINAL)
    Q_PROPERTY(QVariant foreground READ foreground WRITE setForeground RESET resetForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(QVariant background READ background WRITE setBackground RESET resetBackground NOTIFY backgroundChanged FINAL)

    Q_PROPERTY(QColor altHighColor READ altHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altLowColor READ altLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altMediumColor READ altMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altMediumHighColor READ altMediumHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor altMediumLowColor READ altMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseHighColor READ baseHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseLowColor READ baseLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseMediumColor READ baseMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseMediumHighColor READ baseMediumHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor baseMediumLowColor READ baseMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeAltLowColor READ chromeAltLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackHighColor READ chromeBlackHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackLowColor READ chromeBlackLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackMediumLowColor READ chromeBlackMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeBlackMediumColor READ chromeBlackMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeDisabledHighColor READ chromeDisabledHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeDisabledLowColor READ chromeDisabledLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeHighColor READ chromeHighColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeLowColor READ chromeLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeMediumColor READ chromeMediumColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeMediumLowColor READ chromeMediumLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor chromeWhiteColor READ chromeWhiteColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor listLowColor READ listLowColor NOTIFY paletteChanged FINAL)
    Q_PROPERTY(QColor listMediumColor READ listMediumColor NOTIFY paletteChanged FINAL)

    QML_NAMED_ELEMENT(Universal)
    QML_ATTACHED(QQuickUniversalStyle)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickUniversalStyle(QObject *parent = nullptr);

    static QQuickUniversalStyle *qmlAttachedProperties(QObject *object);

    enum Theme { Light, Dark, System };
    Q_ENUM(Theme)

    Theme theme() const;
    void setTheme(Theme theme);
    void inheritTheme(Theme theme);
    void propagateTheme();
    void resetTheme();

    enum Color {
        Lime,
        Green,
        Emerald,
        Teal,
        Cyan,
        Cobalt,
        Indigo,
        Violet,
        Pink,
        Magenta,
        Crimson,
        Red,
        Orange,
        Amber,
        Yellow,
        Brown,
        Olive,
        Steel,
        Mauve,
        Taupe
    };
    Q_ENUM(Color)

    QVariant accent() const;
    void setAccent(const QVariant &accent);
    void inheritAccent(QRgb accent);
    void propagateAccent();
    void resetAccent();

    QVariant foreground() const;
    void setForeground(const QVariant &foreground);
    void inheritForeground(QRgb foreground, bool has);
    void propagateForeground();
    void resetForeground();

    QVariant background() const;
    void setBackground(const QVariant &background);
    void inheritBackground(QRgb background, bool has);
    void propagateBackground();
    void resetBackground();

    Q_INVOKABLE QColor color(Color color) const;

    QColor altHighColor() const;
    QColor altLowColor() const;
    QColor altMediumColor() const;
    QColor altMediumHighColor() const;
    QColor altMediumLowColor() const;
    QColor baseHighColor() const;
    QColor baseLowColor() const;
    QColor baseMediumColor() const;
    QColor baseMediumHighColor() const;
    QColor baseMediumLowColor() const;
    QColor chromeAltLowColor() const;
    QColor chromeBlackHighColor() const;
    QColor chromeBlackLowColor() const;
    QColor chromeBlackMediumLowColor() const;
    QColor chromeBlackMediumColor() const;
    QColor chromeDisabledHighColor() const;
    QColor chromeDisabledLowColor() const;
    QColor chromeHighColor() const;
    QColor chromeLowColor() const;
    QColor chromeMediumColor() const;
    QColor chromeMediumLowColor() const;
    QColor chromeWhiteColor() const;
    QColor listLowColor() const;
    QColor listMediumColor() const;

    enum SystemColor {
        AltHigh,
        AltLow,
        AltMedium,
        AltMediumHigh,
        AltMediumLow,
        BaseHigh,
        BaseLow,
        BaseMedium,
        BaseMediumHigh,
        BaseMediumLow,
        ChromeAltLow,
        ChromeBlackHigh,
        ChromeBlackLow,
        ChromeBlackMediumLow,
        ChromeBlackMedium,
        ChromeDisabledHigh,
        ChromeDisabledLow,
        ChromeHigh,
        ChromeLow,
        ChromeMedium,
        ChromeMediumLow,
        ChromeWhite,
        ListLow,
        ListMedium
    };

    QColor systemColor(SystemColor role) const;

    static void initGlobals();

Q_SIGNALS:
    void themeChanged();
    void accentChanged();
    void foregroundChanged();
    void backgroundChanged();
    void paletteChanged();

protected:
    void attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent) override;

private:
    bool variantToRgba(const QVariant &var, const char *name, QRgb *rgba) const;

    // These reflect whether a color value was explicitly set on the specific
    // item that this attached style object represents.
    bool m_explicitTheme = false;
    bool m_explicitAccent = false;
    bool m_explicitForeground = false;
    bool m_explicitBackground = false;
    // These will be true when this item has an explicit or inherited foreground/background
    // color, or these colors were declared globally via settings (e.g. conf or env vars).
    // Some color properties of the style will return different values depending on whether
    // or not these are set.
    bool m_hasForeground = false;
    bool m_hasBackground = false;
    bool m_usingSystemTheme = false;
    // The actual values for this item, whether explicit, inherited or globally set.
    Theme m_theme = Light;
    QRgb m_accent = Qt::blue;
    QRgb m_foreground = Qt::black;
    QRgb m_background = Qt::white;
};

QT_END_NAMESPACE

#endif // QQUICKUNIVERSALSTYLE_P_H
