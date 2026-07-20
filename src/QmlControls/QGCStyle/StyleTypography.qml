pragma Singleton

import QtQuick

QtObject {
    id: root

    readonly property FontMetrics _bodyFontMetrics: FontMetrics {
        font: root.bodyFont
    }
    readonly property TextMetrics _bodyTextMetrics: TextMetrics {
        font: root.bodyFont
        text: "X"
    }
    readonly property FontMetrics _captionFontMetrics: FontMetrics {
        font: root.captionFont
    }
    readonly property TextMetrics _captionTextMetrics: TextMetrics {
        font: root.captionFont
        text: "X"
    }
    readonly property FontMetrics _headingFontMetrics: FontMetrics {
        font: root.headingFont
    }
    readonly property FontInfo _fixedFontInfo: FontInfo {
        font: SystemFonts.fixedFont
    }
    readonly property TextMetrics _headingTextMetrics: TextMetrics {
        font: root.headingFont
        text: "X"
    }
    readonly property FontMetrics _titleFontMetrics: FontMetrics {
        font: root.titleFont
    }
    readonly property TextMetrics _titleTextMetrics: TextMetrics {
        font: root.titleFont
        text: "X"
    }
    readonly property font bodyFont: Qt.font({
        family: root.normalFontFamily,
        pointSize: root.bodyPointSize
    })
    readonly property real bodyAscent: _bodyFontMetrics.ascent
    readonly property real bodyAverageCharacterWidth: _bodyFontMetrics.averageCharacterWidth
    readonly property real bodyBaselineOffset: bodyAscent
    readonly property real bodyCapitalHeight: _bodyFontMetrics.capitalHeight
    readonly property real bodyDescent: _bodyFontMetrics.descent
    readonly property real bodyFontHeight: _bodyFontMetrics.height
    readonly property real bodyLineSpacing: _bodyFontMetrics.lineSpacing
    readonly property real bodyPixelHeight: Math.round(_bodyFontMetrics.height / 2) * 2
    readonly property real bodyPixelWidth: Math.round(_bodyTextMetrics.advanceWidth / 2) * 2
    readonly property real bodyPointSize: platformPointSize * scaleFactor
    readonly property font captionFont: Qt.font({
        family: root.normalFontFamily,
        pointSize: root.captionPointSize
    })
    readonly property real captionAscent: _captionFontMetrics.ascent
    readonly property real captionBaselineOffset: captionAscent
    readonly property real captionDescent: _captionFontMetrics.descent
    readonly property real captionFontHeight: _captionFontMetrics.height
    readonly property real captionLineSpacing: _captionFontMetrics.lineSpacing
    readonly property real captionPixelHeight: Math.round(_captionFontMetrics.height / 2) * 2
    readonly property real captionPixelWidth: Math.round(_captionTextMetrics.advanceWidth / 2) * 2
    readonly property real captionPointSize: bodyPointSize * captionScale
    readonly property real captionScale: 0.75
    readonly property font fixedFont: Qt.font({
        family: root.fixedFontFamily,
        pointSize: root.bodyPointSize
    })
    readonly property string fixedFontFamily: root._fixedFontInfo.family
    readonly property font headingFont: Qt.font({
        family: root.normalFontFamily,
        pointSize: root.headingPointSize,
        weight: Font.DemiBold
    })
    readonly property real headingAscent: _headingFontMetrics.ascent
    readonly property real headingBaselineOffset: headingAscent
    readonly property real headingDescent: _headingFontMetrics.descent
    readonly property real headingFontHeight: _headingFontMetrics.height
    readonly property real headingLineSpacing: _headingFontMetrics.lineSpacing
    readonly property real headingPixelHeight: Math.round(_headingFontMetrics.height / 2) * 2
    readonly property real headingPixelWidth: Math.round(_headingTextMetrics.advanceWidth / 2) * 2
    readonly property real headingPointSize: bodyPointSize * headingScale
    readonly property real headingScale: 1.25
    property string normalFontFamily: Qt.application.font.family
    readonly property font numericFont: Qt.font({
        family: root.normalFontFamily,
        features: root.tabularNumberFeatures,
        pointSize: root.bodyPointSize
    })
    property real platformPointSize: Qt.application.font.pointSize
    property real scaleFactor: 1
    readonly property var tabularNumberFeatures: ({ "tnum": 1 })
    readonly property font titleFont: Qt.font({
        family: root.normalFontFamily,
        pointSize: root.titlePointSize,
        weight: Font.DemiBold
    })
    readonly property real titleAscent: _titleFontMetrics.ascent
    readonly property real titleBaselineOffset: titleAscent
    readonly property real titleDescent: _titleFontMetrics.descent
    readonly property real titleFontHeight: _titleFontMetrics.height
    readonly property real titleLineSpacing: _titleFontMetrics.lineSpacing
    readonly property real titlePixelHeight: Math.round(_titleFontMetrics.height / 2) * 2
    readonly property real titlePixelWidth: Math.round(_titleTextMetrics.advanceWidth / 2) * 2
    readonly property real titlePointSize: bodyPointSize * titleScale
    readonly property real titleScale: 1.5
}
