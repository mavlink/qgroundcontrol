// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMATERIALSTYLE_P_H
#define QQUICKMATERIALSTYLE_P_H

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
#include <QtQuickControls2Material/qtquickcontrols2materialexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2MATERIAL_EXPORT QQuickMaterialStyle : public QQuickAttachedPropertyPropagator
{
    Q_OBJECT
    Q_PROPERTY(Theme theme READ theme WRITE setTheme RESET resetTheme NOTIFY themeChanged FINAL)
    Q_PROPERTY(QVariant primary READ primary WRITE setPrimary RESET resetPrimary NOTIFY primaryChanged FINAL)
    Q_PROPERTY(QVariant accent READ accent WRITE setAccent RESET resetAccent NOTIFY accentChanged FINAL)
    Q_PROPERTY(QVariant foreground READ foreground WRITE setForeground RESET resetForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(QVariant background READ background WRITE setBackground RESET resetBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(int elevation READ elevation WRITE setElevation RESET resetElevation NOTIFY elevationChanged FINAL)
    Q_PROPERTY(RoundedScale roundedScale READ roundedScale WRITE setRoundedScale RESET resetRoundedScale
        NOTIFY roundedScaleChanged FINAL)
    Q_PROPERTY(ContainerStyle containerStyle READ containerStyle WRITE setContainerStyle RESET resetContainerStyle
        NOTIFY containerStyleChanged FINAL)

    Q_PROPERTY(QColor primaryColor READ primaryColor NOTIFY primaryChanged FINAL) // TODO: remove?
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentChanged FINAL) // TODO: remove?
    Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QColor primaryTextColor READ primaryTextColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor primaryHighlightedTextColor READ primaryHighlightedTextColor NOTIFY primaryHighlightedTextColorChanged FINAL)
    Q_PROPERTY(QColor secondaryTextColor READ secondaryTextColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor hintTextColor READ hintTextColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor textSelectionColor READ textSelectionColor NOTIFY themeOrAccentChanged FINAL)
    Q_PROPERTY(QColor dropShadowColor READ dropShadowColor CONSTANT FINAL)
    Q_PROPERTY(QColor dividerColor READ dividerColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor iconColor READ iconColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor iconDisabledColor READ iconDisabledColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor frameColor READ frameColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor rippleColor READ rippleColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor highlightedRippleColor READ highlightedRippleColor NOTIFY themeOrAccentChanged FINAL)
    Q_PROPERTY(QColor switchUncheckedTrackColor READ switchUncheckedTrackColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchCheckedTrackColor READ switchCheckedTrackColor NOTIFY themeOrAccentChanged FINAL)
    Q_PROPERTY(QColor switchUncheckedHandleColor READ switchUncheckedHandleColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchUncheckedHoveredHandleColor READ switchUncheckedHoveredHandleColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchDisabledUncheckedTrackColor READ switchDisabledUncheckedTrackColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchDisabledCheckedTrackColor READ switchDisabledCheckedTrackColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchDisabledUncheckedTrackBorderColor READ switchDisabledUncheckedTrackBorderColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchCheckedHandleColor READ switchCheckedHandleColor NOTIFY themeOrAccentChanged FINAL)
    Q_PROPERTY(QColor switchDisabledUncheckedHandleColor READ switchDisabledUncheckedHandleColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchDisabledCheckedHandleColor READ switchDisabledCheckedHandleColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchDisabledCheckedIconColor READ switchDisabledCheckedIconColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor switchDisabledUncheckedIconColor READ switchDisabledUncheckedIconColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor scrollBarColor READ scrollBarColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor scrollBarHoveredColor READ scrollBarHoveredColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor scrollBarPressedColor READ scrollBarPressedColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor dialogColor READ dialogColor NOTIFY dialogColorChanged FINAL)
    Q_PROPERTY(QColor backgroundDimColor READ backgroundDimColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor listHighlightColor READ listHighlightColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor tooltipColor READ tooltipColor NOTIFY tooltipColorChanged FINAL)
    Q_PROPERTY(QColor toolBarColor READ toolBarColor NOTIFY toolBarColorChanged FINAL)
    Q_PROPERTY(QColor toolTextColor READ toolTextColor NOTIFY toolTextColorChanged FINAL)
    Q_PROPERTY(QColor spinBoxDisabledIconColor READ spinBoxDisabledIconColor NOTIFY themeChanged FINAL)
    Q_PROPERTY(QColor sliderDisabledColor READ sliderDisabledColor NOTIFY themeChanged FINAL REVISION(2, 15))
    Q_PROPERTY(QColor textFieldFilledContainerColor READ textFieldFilledContainerColor NOTIFY themeChanged FINAL)

    Q_PROPERTY(int touchTarget READ touchTarget CONSTANT FINAL)
    Q_PROPERTY(int buttonVerticalPadding READ buttonVerticalPadding CONSTANT FINAL)
    Q_PROPERTY(int buttonHeight READ buttonHeight CONSTANT FINAL)
    Q_PROPERTY(int delegateHeight READ delegateHeight CONSTANT FINAL)
    Q_PROPERTY(int dialogButtonBoxHeight READ dialogButtonBoxHeight CONSTANT FINAL)
    Q_PROPERTY(int dialogTitleFontPixelSize READ dialogTitleFontPixelSize CONSTANT FINAL)
    Q_PROPERTY(RoundedScale dialogRoundedScale READ dialogRoundedScale CONSTANT FINAL)
    Q_PROPERTY(int frameVerticalPadding READ frameVerticalPadding CONSTANT FINAL)
    Q_PROPERTY(int menuItemHeight READ menuItemHeight CONSTANT FINAL)
    Q_PROPERTY(int menuItemVerticalPadding READ menuItemVerticalPadding CONSTANT FINAL)
    Q_PROPERTY(int switchIndicatorWidth READ switchIndicatorWidth CONSTANT FINAL)
    Q_PROPERTY(int switchIndicatorHeight READ switchIndicatorHeight CONSTANT FINAL)
    Q_PROPERTY(int switchNormalHandleHeight READ switchNormalHandleHeight CONSTANT FINAL)
    Q_PROPERTY(int switchCheckedHandleHeight READ switchCheckedHandleHeight CONSTANT FINAL)
    Q_PROPERTY(int switchLargestHandleHeight READ switchLargestHandleHeight CONSTANT FINAL)
    Q_PROPERTY(int switchDelegateVerticalPadding READ switchDelegateVerticalPadding CONSTANT FINAL)
    Q_PROPERTY(int textFieldHeight READ textFieldHeight CONSTANT FINAL)
    Q_PROPERTY(int textFieldHorizontalPadding READ textFieldHorizontalPadding CONSTANT FINAL)
    Q_PROPERTY(int textFieldVerticalPadding READ textFieldVerticalPadding CONSTANT FINAL)
    Q_PROPERTY(int tooltipHeight READ tooltipHeight CONSTANT FINAL)

    QML_NAMED_ELEMENT(Material)
    QML_ATTACHED(QQuickMaterialStyle)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum Theme {
        Light,
        Dark,
        System
    };

    enum Variant {
        Normal,
        Dense
    };

    enum Color {
        Red,
        Pink,
        Purple,
        DeepPurple,
        Indigo,
        Blue,
        LightBlue,
        Cyan,
        Teal,
        Green,
        LightGreen,
        Lime,
        Yellow,
        Amber,
        Orange,
        DeepOrange,
        Brown,
        Grey,
        BlueGrey
    };

    enum Shade {
        Shade50,
        Shade100,
        Shade200,
        Shade300,
        Shade400,
        Shade500,
        Shade600,
        Shade700,
        Shade800,
        Shade900,
        ShadeA100,
        ShadeA200,
        ShadeA400,
        ShadeA700,
    };

    enum class RoundedScale {
        NotRounded,
        ExtraSmallScale = 4,
        SmallScale = 8,
        MediumScale = 12,
        LargeScale = 16,
        ExtraLargeScale = 28,
        FullScale = 0xFF // For full we use half the height of the item.
    };

    enum class ContainerStyle {
        Filled,
        Outlined
    };

    Q_ENUM(Theme)
    Q_ENUM(Variant)
    Q_ENUM(Color)
    Q_ENUM(Shade)
    Q_ENUM(RoundedScale)
    Q_ENUM(ContainerStyle)

    explicit QQuickMaterialStyle(QObject *parent = nullptr);

    static QQuickMaterialStyle *qmlAttachedProperties(QObject *object);

    Theme theme() const;
    void setTheme(Theme theme);
    void inheritTheme(Theme theme);
    void propagateTheme();
    void resetTheme();
    void themeChange();

    QVariant primary() const;
    void setPrimary(const QVariant &accent);
    void inheritPrimary(uint primary, bool custom);
    void propagatePrimary();
    void resetPrimary();
    void primaryChange();

    QVariant accent() const;
    void setAccent(const QVariant &accent);
    void inheritAccent(uint accent, bool custom);
    void propagateAccent();
    void resetAccent();
    void accentChange();

    QVariant foreground() const;
    void setForeground(const QVariant &foreground);
    void inheritForeground(uint foreground, bool custom, bool has);
    void propagateForeground();
    void resetForeground();
    void foregroundChange();

    QVariant background() const;
    void setBackground(const QVariant &background);
    void inheritBackground(uint background, bool custom, bool has);
    void propagateBackground();
    void resetBackground();
    void backgroundChange();

    int elevation() const;
    void setElevation(int elevation);
    void resetElevation();
    void elevationChange();

    RoundedScale roundedScale() const;
    void setRoundedScale(RoundedScale roundedScale);
    void resetRoundedScale();

    ContainerStyle containerStyle() const;
    void setContainerStyle(ContainerStyle containerStyle);
    void resetContainerStyle();

    QColor primaryColor() const;
    QColor accentColor() const;
    QColor backgroundColor() const;
    QColor primaryTextColor() const;
    QColor primaryHighlightedTextColor() const;
    QColor secondaryTextColor() const;
    QColor hintTextColor() const;
    QColor textSelectionColor() const;
    QColor dropShadowColor() const;
    QColor dividerColor() const;
    QColor iconColor() const;
    QColor iconDisabledColor() const;
    Q_INVOKABLE QColor buttonColor(Theme theme, const QVariant &background, const QVariant &accent,
        bool enabled, bool flat, bool highlighted, bool checked) const;
    QColor frameColor() const;
    QColor rippleColor() const;
    QColor highlightedRippleColor() const;
    QColor switchUncheckedTrackColor() const;
    QColor switchCheckedTrackColor() const;
    QColor switchDisabledUncheckedTrackColor() const;
    QColor switchDisabledCheckedTrackColor() const;
    QColor switchDisabledUncheckedTrackBorderColor() const;
    QColor switchUncheckedHandleColor() const;
    QColor switchUncheckedHoveredHandleColor() const;
    QColor switchCheckedHandleColor() const;
    QColor switchDisabledUncheckedHandleColor() const;
    QColor switchDisabledCheckedHandleColor() const;
    QColor switchDisabledCheckedIconColor() const;
    QColor switchDisabledUncheckedIconColor() const;
    QColor scrollBarColor() const;
    QColor scrollBarHoveredColor() const;
    QColor scrollBarPressedColor() const;
    QColor dialogColor() const;
    QColor backgroundDimColor() const;
    QColor listHighlightColor() const;
    QColor tooltipColor() const;
    QColor toolBarColor() const;
    QColor toolTextColor() const;
    QColor spinBoxDisabledIconColor() const;
    QColor sliderDisabledColor() const;
    QColor textFieldFilledContainerColor() const;

    Q_INVOKABLE QColor color(Color color, Shade shade = Shade500) const;
    Q_INVOKABLE QColor shade(const QColor &color, Shade shade) const;

    int touchTarget() const;
    int buttonVerticalPadding() const;
    Q_INVOKABLE int buttonLeftPadding(bool flat, bool hasIcon) const;
    Q_INVOKABLE int buttonRightPadding(bool flat, bool hasIcon, bool hasText) const;
    int buttonHeight() const;
    int delegateHeight() const;
    int dialogButtonBoxHeight() const;
    int dialogTitleFontPixelSize() const;
    RoundedScale dialogRoundedScale() const;
    int frameVerticalPadding() const;
    int menuItemHeight() const;
    int menuItemVerticalPadding() const;
    int switchIndicatorWidth() const;
    int switchIndicatorHeight() const;
    int switchNormalHandleHeight() const;
    int switchCheckedHandleHeight() const;
    int switchLargestHandleHeight() const;
    int switchDelegateVerticalPadding() const;
    int textFieldHeight() const;
    int textFieldHorizontalPadding() const;
    int textFieldVerticalPadding() const;
    int tooltipHeight() const;

    static void initGlobals();

    static Variant variant();

Q_SIGNALS:
    void themeChanged();
    void primaryChanged();
    void accentChanged();
    void foregroundChanged();
    void backgroundChanged();
    void elevationChanged();

    void themeOrAccentChanged();

    void primaryHighlightedTextColorChanged();
    void dialogColorChanged();
    void tooltipColorChanged();
    void toolBarColorChanged();
    void toolTextColorChanged();
    void roundedScaleChanged();
    void containerStyleChanged();

protected:
    void attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent) override;

private:
    void init();
    bool variantToRgba(const QVariant &var, const char *name, QRgb *rgba, bool *custom) const;

    QColor backgroundColor(Shade shade) const;
    QColor accentColor(Shade shade) const;

    Shade themeShade() const;

    // These reflect whether a color value was explicitly set on the specific
    // item that this attached style object represents.
    bool m_explicitTheme = false;
    bool m_explicitPrimary = false;
    bool m_explicitAccent = false;
    bool m_explicitForeground = false;
    bool m_explicitBackground = false;
    // These reflect whether the color value that was either inherited or
    // explicitly set is in the form that QColor expects, rather than one of
    // our pre-defined color enum values.
    bool m_customPrimary = false;
    bool m_customAccent = false;
    bool m_customForeground = false;
    bool m_customBackground = false;
    // These will be true when this item has an explicit or inherited foreground/background
    // color, or these colors were declared globally via settings (e.g. conf or env vars).
    // Some color properties of the style will return different values depending on whether
    // or not these are set.
    bool m_hasForeground = false;
    bool m_hasBackground = false;
    // The actual values for this item, whether explicit, inherited or globally set.
    bool m_systemTheme = false;
    Theme m_theme = Light;
    uint m_primary = 0;
    uint m_accent = 0;
    uint m_foreground = 0;
    uint m_background = 0;
    int m_elevation = 0;
    RoundedScale m_roundedScale = RoundedScale::NotRounded;
    ContainerStyle m_containerStyle = ContainerStyle::Filled;
};

QT_END_NAMESPACE

#endif // QQUICKMATERIALSTYLE_P_H
