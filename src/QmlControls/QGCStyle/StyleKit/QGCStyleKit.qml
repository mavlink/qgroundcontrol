pragma ComponentBehavior: Bound

import Qt.labs.StyleKit as Labs

import QtQuick

Labs.Style {
    id: rootStyle

    themeName: "QGCPalette"

    Labs.CustomTheme {
        name: "QGCPalette"
        theme: Labs.Theme {
        }
    }

    fonts {
        system {
            family: StyleTypography.normalFontFamily
            pointSize: StyleTypography.bodyPointSize
        }
    }

    control {
        checked.background.color: palette.mid
        checked.indicator.foreground.visible: true
        focused.background.border.color: palette.highlight
        focused.background.border.width: StyleMetrics.focusBorderWidth
        hovered.background.color: palette.midlight
        padding: StyleMetrics.controlVerticalPadding
        pressed.background.color: palette.mid
        spacing: StyleMetrics.spacing
        text.color: palette.buttonText
        transition: Transition {
            Labs.StyleAnimation {
                animateColors: true
                animateBackgroundBorder: true
                duration: StyleMetrics.normalAnimationDuration
                easing.type: Easing.OutCubic
            }
        }

        background {
            border.color: palette.mid
            border.width: StylePreferences.highContrast ? StyleMetrics.focusBorderWidth : StyleMetrics.borderWidth
            color: palette.button
            implicitHeight: StyleMetrics.controlHeight
            implicitWidth: StyleMetrics.controlWidth
            radius: StyleMetrics.radius
        }

        handle {
            border.color: palette.mid
            border.width: StyleMetrics.borderWidth
            color: palette.window
            implicitHeight: StyleMetrics.indicatorSize
            implicitWidth: StyleMetrics.indicatorSize
            radius: StyleMetrics.indicatorSize / 2
        }

        indicator {
            border.color: palette.mid
            border.width: StyleMetrics.borderWidth
            color: palette.base
            foreground.color: palette.highlight
            implicitHeight: StyleMetrics.indicatorSize
            implicitWidth: StyleMetrics.indicatorSize
            radius: StyleMetrics.radius
        }
    }

    Labs.StyleVariation {
        name: "primary"

        control {
            background.color: rootStyle.palette.highlight
            checked.background.color: rootStyle.palette.highlight
            hovered.background.color: Qt.lighter(rootStyle.palette.highlight, 1.05)
            pressed.background.color: Qt.darker(rootStyle.palette.highlight, 1.1)
            text.bold: true
            text.color: rootStyle.palette.highlightedText
        }
    }

    Labs.StyleVariation {
        name: "compact"

        control {
            background.implicitHeight: StyleMetrics.toolbarIconSize + StyleMetrics.spacing
            handle.implicitHeight: StyleMetrics.toolbarIconSize
            handle.implicitWidth: StyleMetrics.toolbarIconSize
            indicator.implicitHeight: StyleMetrics.toolbarIconSize
            indicator.implicitWidth: StyleMetrics.toolbarIconSize
            padding: StyleMetrics.smallSpacing
        }

        button {
            leftPadding: StyleMetrics.spacing
            rightPadding: StyleMetrics.spacing
        }
    }

    button {
        leftPadding: StyleMetrics.controlHorizontalPadding
        rightPadding: StyleMetrics.controlHorizontalPadding
    }

    checkBox {
        background.visible: false
        checked.indicator.foreground.visible: true
        indicator.foreground.visible: false
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
    }

    comboBox {
        background.implicitWidth: StyleMetrics.controlWidth + StyleMetrics.iconSize
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
    }

    itemDelegate {
        background.border.width: 0
        background.color: "transparent"
        background.radius: StyleMetrics.radius
        hovered.background.color: palette.midlight
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
    }

    label {
        background.visible: false
        text.color: palette.windowText
    }

    pane {
        background.color: palette.window
        background.radius: StyleMetrics.panelRadius
        padding: StyleMetrics.contentMargin
    }

    popup {
        background.color: palette.window
        background.implicitWidth: StyleMetrics.menuWidth
        background.radius: StyleMetrics.panelRadius
        padding: StyleMetrics.contentMargin
    }

    progressBar {
        background.visible: false
        indicator.implicitHeight: StyleMetrics.progressBarHeight
        indicator.implicitWidth: StyleMetrics.menuWidth
        indicator.radius: StyleMetrics.progressBarHeight / 2
    }

    radioButton {
        background.visible: false
        checked.indicator.foreground.visible: true
        indicator.foreground.margins: StyleMetrics.padding
        indicator.foreground.radius: StyleMetrics.indicatorSize / 2
        indicator.foreground.visible: false
        indicator.radius: StyleMetrics.indicatorSize / 2
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
    }

    scrollBar {
        background.implicitHeight: StyleMetrics.scrollBarExtent
        background.visible: StylePreferences.highContrast
        indicator.foreground.color: palette.mid
        indicator.foreground.radius: StyleMetrics.scrollBarExtent / 2
        indicator.implicitHeight: StyleMetrics.scrollBarExtent
        padding: StyleMetrics.borderWidth * 2

        vertical {
            background.implicitWidth: StyleMetrics.scrollBarExtent
            indicator.implicitWidth: StyleMetrics.scrollBarExtent
        }
    }

    scrollIndicator {
        background.implicitHeight: StyleMetrics.scrollBarExtent
        background.visible: StylePreferences.highContrast
        indicator.border.width: 0
        indicator.foreground.color: palette.mid
        indicator.foreground.radius: StyleMetrics.scrollBarExtent / 2
        indicator.implicitHeight: StyleMetrics.scrollBarExtent

        vertical {
            background.implicitWidth: StyleMetrics.scrollBarExtent
            indicator.implicitWidth: StyleMetrics.scrollBarExtent
        }
    }

    slider {
        background.visible: false
        indicator.foreground.radius: StyleMetrics.progressBarHeight / 2
        indicator.implicitHeight: StyleMetrics.progressBarHeight
        indicator.implicitWidth: Labs.Style.Stretch
        indicator.radius: StyleMetrics.progressBarHeight / 2
    }

    switchControl {
        background.visible: false
        checked.indicator.foreground.color: palette.highlight
        indicator.implicitHeight: StyleMetrics.switchHeight
        indicator.implicitWidth: StyleMetrics.switchWidth
        indicator.radius: StyleMetrics.switchHeight / 2
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
    }

    textArea {
        background.color: palette.base
        background.implicitHeight: StyleMetrics.controlHeight * 2
        background.implicitWidth: StyleMetrics.menuWidth
        text.color: palette.text
    }

    textField {
        background.color: palette.base
        background.implicitWidth: StyleMetrics.menuWidth
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        text.color: palette.text
    }
}
