// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXT_P_H
#define QQUICKTEXT_P_H

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

#include "qquickimplicitsizeitem_p.h"
#include "qquicktextinterface_p.h"
#include <private/qtquickglobal_p.h>
#include <QtGui/qtextoption.h>

QT_BEGIN_NAMESPACE

class QQuickTextPrivate;
class QQuickTextLine;
class Q_QUICK_EXPORT QQuickText : public QQuickImplicitSizeItem, public QQuickTextInterface
{
    Q_OBJECT
    Q_INTERFACES(QQuickTextInterface)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor linkColor READ linkColor WRITE setLinkColor NOTIFY linkColorChanged)
    Q_PROPERTY(TextStyle style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(QColor styleColor READ styleColor WRITE setStyleColor NOTIFY styleColorChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ hAlign WRITE setHAlign RESET resetHAlign NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(HAlignment effectiveHorizontalAlignment READ effectiveHAlign NOTIFY effectiveHorizontalAlignmentChanged)
    Q_PROPERTY(VAlignment verticalAlignment READ vAlign WRITE setVAlign NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY lineCountChanged)
    Q_PROPERTY(bool truncated READ truncated NOTIFY truncatedChanged)
    Q_PROPERTY(int maximumLineCount READ maximumLineCount WRITE setMaximumLineCount NOTIFY maximumLineCountChanged RESET resetMaximumLineCount)

    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
    Q_PROPERTY(TextElideMode elide READ elideMode WRITE setElideMode NOTIFY elideModeChanged) //### elideMode?
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(qreal paintedWidth READ contentWidth NOTIFY contentWidthChanged)  // Compatibility
    Q_PROPERTY(qreal paintedHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(qreal lineHeight READ lineHeight WRITE setLineHeight NOTIFY lineHeightChanged)
    Q_PROPERTY(LineHeightMode lineHeightMode READ lineHeightMode WRITE setLineHeightMode NOTIFY lineHeightModeChanged)
    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl RESET resetBaseUrl NOTIFY baseUrlChanged)
    Q_PROPERTY(int minimumPixelSize READ minimumPixelSize WRITE setMinimumPixelSize NOTIFY minimumPixelSizeChanged)
    Q_PROPERTY(int minimumPointSize READ minimumPointSize WRITE setMinimumPointSize NOTIFY minimumPointSizeChanged)
    Q_PROPERTY(FontSizeMode fontSizeMode READ fontSizeMode WRITE setFontSizeMode NOTIFY fontSizeModeChanged)
    Q_PROPERTY(RenderType renderType READ renderType WRITE setRenderType NOTIFY renderTypeChanged)
    Q_PROPERTY(QString hoveredLink READ hoveredLink NOTIFY linkHovered REVISION(2, 2))
    Q_PROPERTY(int renderTypeQuality READ renderTypeQuality WRITE setRenderTypeQuality NOTIFY renderTypeQualityChanged REVISION(6, 0))

    Q_PROPERTY(qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY topPaddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY leftPaddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding NOTIFY rightPaddingChanged REVISION(2, 6))
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding NOTIFY bottomPaddingChanged REVISION(2, 6))

    Q_PROPERTY(QJSValue fontInfo READ fontInfo NOTIFY fontInfoChanged REVISION(2, 9))
    Q_PROPERTY(QSizeF advance READ advance NOTIFY contentSizeChanged REVISION(2, 10))
    QML_NAMED_ELEMENT(Text)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickText(QQuickItem *parent=nullptr);
    ~QQuickText() override;

    enum HAlignment { AlignLeft = Qt::AlignLeft,
                       AlignRight = Qt::AlignRight,
                       AlignHCenter = Qt::AlignHCenter,
                       AlignJustify = Qt::AlignJustify };
    Q_ENUM(HAlignment)
    enum VAlignment { AlignTop = Qt::AlignTop,
                       AlignBottom = Qt::AlignBottom,
                       AlignVCenter = Qt::AlignVCenter };
    Q_ENUM(VAlignment)
    enum TextStyle { Normal,
                      Outline,
                      Raised,
                      Sunken };
    Q_ENUM(TextStyle)
    enum TextFormat { PlainText = Qt::PlainText,
                       RichText = Qt::RichText,
                       MarkdownText = Qt::MarkdownText,
                       AutoText = Qt::AutoText,
                       StyledText = 4 };
    Q_ENUM(TextFormat)
    enum TextElideMode { ElideLeft = Qt::ElideLeft,
                          ElideRight = Qt::ElideRight,
                          ElideMiddle = Qt::ElideMiddle,
                          ElideNone = Qt::ElideNone };
    Q_ENUM(TextElideMode)

    enum WrapMode { NoWrap = QTextOption::NoWrap,
                    WordWrap = QTextOption::WordWrap,
                    WrapAnywhere = QTextOption::WrapAnywhere,
                    WrapAtWordBoundaryOrAnywhere = QTextOption::WrapAtWordBoundaryOrAnywhere, // COMPAT
                    Wrap = QTextOption::WrapAtWordBoundaryOrAnywhere
                  };
    Q_ENUM(WrapMode)

    enum RenderType { QtRendering,
                      NativeRendering,
                      CurveRendering
                    };
    Q_ENUM(RenderType)

    enum RenderTypeQuality { DefaultRenderTypeQuality = -1,
                             LowRenderTypeQuality = 26,
                             NormalRenderTypeQuality = 52,
                             HighRenderTypeQuality = 104,
                             VeryHighRenderTypeQuality = 208
                           };
    Q_ENUM(RenderTypeQuality)

    enum LineHeightMode { ProportionalHeight, FixedHeight };
    Q_ENUM(LineHeightMode)

    enum FontSizeMode { FixedSize = 0x0, HorizontalFit = 0x01, VerticalFit = 0x02,
                        Fit = HorizontalFit | VerticalFit };
    Q_ENUM(FontSizeMode)

    QString text() const;
    void setText(const QString &);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &c);

    QColor linkColor() const;
    void setLinkColor(const QColor &color);

    TextStyle style() const;
    void setStyle(TextStyle style);

    QColor styleColor() const;
    void setStyleColor(const QColor &c);

    HAlignment hAlign() const;
    void setHAlign(HAlignment align);
    void resetHAlign();
    HAlignment effectiveHAlign() const;

    VAlignment vAlign() const;
    void setVAlign(VAlignment align);

    WrapMode wrapMode() const;
    void setWrapMode(WrapMode w);

    int lineCount() const;
    bool truncated() const;

    int maximumLineCount() const;
    void setMaximumLineCount(int lines);
    void resetMaximumLineCount();

    TextFormat textFormat() const;
    void setTextFormat(TextFormat format);

    TextElideMode elideMode() const;
    void setElideMode(TextElideMode);

    qreal lineHeight() const;
    void setLineHeight(qreal lineHeight);

    LineHeightMode lineHeightMode() const;
    void setLineHeightMode(LineHeightMode);


    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &url);
    void resetBaseUrl();

    int minimumPixelSize() const;
    void setMinimumPixelSize(int size);

    int minimumPointSize() const;
    void setMinimumPointSize(int size);

    FontSizeMode fontSizeMode() const;
    void setFontSizeMode(FontSizeMode mode);

    void componentComplete() override;

    int resourcesLoading() const; // mainly for testing

    qreal contentWidth() const;
    qreal contentHeight() const;

    QRectF boundingRect() const override;
    QRectF clipRect() const override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_X("Use forceLayout() instead")
    Q_INVOKABLE void doLayout();
#endif
#endif
    Q_REVISION(2, 9) Q_INVOKABLE void forceLayout();

    RenderType renderType() const;
    void setRenderType(RenderType renderType);

    int renderTypeQuality() const;
    void setRenderTypeQuality(int renderTypeQuality);

    QString hoveredLink() const;

    Q_REVISION(2, 3) Q_INVOKABLE QString linkAt(qreal x, qreal y) const;

    qreal padding() const;
    void setPadding(qreal padding);
    void resetPadding();

    qreal topPadding() const;
    void setTopPadding(qreal padding);
    void resetTopPadding();

    qreal leftPadding() const;
    void setLeftPadding(qreal padding);
    void resetLeftPadding();

    qreal rightPadding() const;
    void setRightPadding(qreal padding);
    void resetRightPadding();

    qreal bottomPadding() const;
    void setBottomPadding(qreal padding);
    void resetBottomPadding();

    QJSValue fontInfo() const;
    QSizeF advance() const;

    void invalidate() override;

Q_SIGNALS:
    void textChanged(const QString &text);
    void linkActivated(const QString &link);
    Q_REVISION(2, 2) void linkHovered(const QString &link);
    void fontChanged(const QFont &font);
    void colorChanged();
    void linkColorChanged();
    void styleChanged(QQuickText::TextStyle style);
    void styleColorChanged();
    void horizontalAlignmentChanged(QQuickText::HAlignment alignment);
    void verticalAlignmentChanged(QQuickText::VAlignment alignment);
    void wrapModeChanged();
    void lineCountChanged();
    void truncatedChanged();
    void maximumLineCountChanged();
    void textFormatChanged(QQuickText::TextFormat textFormat);
    void elideModeChanged(QQuickText::TextElideMode mode);
    void contentSizeChanged();
    // The next two signals should be marked as Q_REVISION(2, 12). See QTBUG-71247
    void contentWidthChanged(qreal contentWidth);
    void contentHeightChanged(qreal contentHeight);

    void lineHeightChanged(qreal lineHeight);
    void lineHeightModeChanged(QQuickText::LineHeightMode mode);
    void fontSizeModeChanged();
    void minimumPixelSizeChanged();
    void minimumPointSizeChanged();
    void effectiveHorizontalAlignmentChanged();
    void lineLaidOut(QQuickTextLine *line);
    void baseUrlChanged();
    void renderTypeChanged();
    Q_REVISION(2, 6) void paddingChanged();
    Q_REVISION(2, 6) void topPaddingChanged();
    Q_REVISION(2, 6) void leftPaddingChanged();
    Q_REVISION(2, 6) void rightPaddingChanged();
    Q_REVISION(2, 6) void bottomPaddingChanged();
    Q_REVISION(2, 9) void fontInfoChanged();
    Q_REVISION(6, 0) void renderTypeQualityChanged();

protected:
    QQuickText(QQuickTextPrivate &dd, QQuickItem *parent = nullptr);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    void updatePolish() override;

    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void invalidateFontCaches();

private Q_SLOTS:
    void q_updateLayout();
    void triggerPreprocess();
    Q_REVISION(6, 7) QVariant loadResource(int type, const QUrl &source);
    void resourceRequestFinished();
    void imageDownloadFinished();

private:
    Q_DISABLE_COPY(QQuickText)
    Q_DECLARE_PRIVATE(QQuickText)
};

Q_DECLARE_MIXED_ENUM_OPERATORS_SYMMETRIC(int, QQuickText::HAlignment, QQuickText::VAlignment)

class QTextLine;
class Q_QUICK_EXPORT QQuickTextLine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int number READ number FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight FINAL)
    Q_PROPERTY(qreal x READ x WRITE setX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY FINAL)
    Q_PROPERTY(qreal implicitWidth READ implicitWidth REVISION(2, 15) FINAL)
    Q_PROPERTY(bool isLast READ isLast REVISION(2, 15) FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickTextLine();

    void setLine(QTextLine* line);
    void setLineOffset(int offset);
    void setFullLayoutTextLength(int length);
    int number() const;
    qreal implicitWidth() const;
    bool isLast() const;

    qreal width() const;
    void setWidth(qreal width);

    qreal height() const;
    void setHeight(qreal height);

    qreal x() const;
    void setX(qreal x);

    qreal y() const;
    void setY(qreal y);

private:
    QTextLine *m_line;
    qreal m_height;
    int m_lineOffset;
    int m_fullLayoutTextLength;
};

QT_END_NAMESPACE

#endif // QQUICKTEXT_P_H
