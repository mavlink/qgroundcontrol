// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMATERIALTEXTCONTAINER_P_H
#define QQUICKMATERIALTEXTCONTAINER_P_H

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

#include <QtCore/qpointer.h>
#include <QtCore/qpropertyanimation.h>
#include <QtCore/private/qglobal_p.h>
#include <QtGui/qcolor.h>
#include <QtQuick/qquickpainteditem.h>

QT_BEGIN_NAMESPACE

class QQuickMaterialTextContainer : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(bool filled READ isFilled WRITE setFilled FINAL)
    Q_PROPERTY(bool controlHasActiveFocus READ controlHasActiveFocus
        WRITE setControlHasActiveFocus NOTIFY controlHasActiveFocusChanged FINAL)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor FINAL)
    Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor FINAL)
    Q_PROPERTY(QColor focusedOutlineColor READ focusedOutlineColor WRITE setFocusedOutlineColor FINAL)
    Q_PROPERTY(qreal focusAnimationProgress READ focusAnimationProgress WRITE setFocusAnimationProgress FINAL)
    Q_PROPERTY(qreal placeholderTextWidth READ placeholderTextWidth WRITE setPlaceholderTextWidth FINAL)
    Q_PROPERTY(PlaceHolderHAlignment placeholderTextHAlign READ placeholderTextHAlign WRITE setPlaceholderTextHAlign FINAL)
    Q_PROPERTY(bool controlHasText READ controlHasText WRITE setControlHasText NOTIFY controlHasTextChanged FINAL)
    Q_PROPERTY(bool placeholderHasText READ placeholderHasText WRITE setPlaceholderHasText NOTIFY placeholderHasTextChanged FINAL)
    Q_PROPERTY(int horizontalPadding READ horizontalPadding WRITE setHorizontalPadding NOTIFY horizontalPaddingChanged FINAL)
    QML_NAMED_ELEMENT(MaterialTextContainer)
    QML_ADDED_IN_VERSION(6, 5)

public:
    explicit QQuickMaterialTextContainer(QQuickItem *parent = nullptr);

    enum PlaceHolderHAlignment {
        AlignLeft = Qt::AlignLeft,
        AlignRight = Qt::AlignRight,
        AlignHCenter = Qt::AlignHCenter,
        AlignJustify = Qt::AlignJustify
    };
    Q_ENUM(PlaceHolderHAlignment)

    bool isFilled() const;
    void setFilled(bool filled);

    QColor fillColor() const;
    void setFillColor(const QColor &fillColor);

    QColor outlineColor() const;
    void setOutlineColor(const QColor &outlineColor);

    QColor focusedOutlineColor() const;
    void setFocusedOutlineColor(const QColor &focusedOutlineColor);

    qreal focusAnimationProgress() const;
    void setFocusAnimationProgress(qreal progress);

    qreal placeholderTextWidth() const;
    void setPlaceholderTextWidth(qreal placeholderTextWidth);

    bool controlHasActiveFocus() const;
    void setControlHasActiveFocus(bool controlHasActiveFocus);

    bool controlHasText() const;
    void setControlHasText(bool controlHasText);

    bool placeholderHasText() const;
    void setPlaceholderHasText(bool placeholderHasText);

    int horizontalPadding() const;
    void setHorizontalPadding(int horizontalPadding);

    void paint(QPainter *painter) override;

    PlaceHolderHAlignment placeholderTextHAlign() const;
    void setPlaceholderTextHAlign(PlaceHolderHAlignment placeHolderTextHAlign);

signals:
    void animateChanged();
    void controlHasActiveFocusChanged();
    void controlHasTextChanged();
    void placeholderHasTextChanged();
    void horizontalPaddingChanged();

private:
    bool shouldAnimateOutline() const;

    QQuickItem *textControl() const;
    void controlGotActiveFocus();
    void controlLostActiveFocus();

    void updateFocusAnimation(bool createIfNeeded = false);

    void componentComplete() override;

    QColor m_fillColor;
    QColor m_outlineColor;
    QColor m_focusedOutlineColor;
    qreal m_focusAnimationProgress = 0;
    qreal m_placeholderTextWidth = 0;
    bool m_filled = false;
    bool m_controlHasActiveFocus = false;
    bool m_controlHasText = false;
    bool m_placeholderHasText = false;
    int m_horizontalPadding = 0;
    PlaceHolderHAlignment m_placeholderTextHAlign;
    QPointer<QPropertyAnimation> m_focusAnimation;
};

QT_END_NAMESPACE

#endif // QQUICKMATERIALTEXTCONTAINER_P_H
