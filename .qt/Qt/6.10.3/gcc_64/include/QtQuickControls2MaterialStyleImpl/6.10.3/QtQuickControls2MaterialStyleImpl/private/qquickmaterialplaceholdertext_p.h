// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMATERIALPLACEHOLDERTEXT_P_H
#define QQUICKMATERIALPLACEHOLDERTEXT_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtGui/qcolor.h>
#include <QtQuickControls2Impl/private/qquickplaceholdertext_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QParallelAnimationGroup;

class QQuickMaterialPlaceholderText : public QQuickPlaceholderText
{
    Q_OBJECT
    Q_PROPERTY(bool filled READ isFilled WRITE setFilled NOTIFY filledChanged FINAL)
    Q_PROPERTY(bool controlHasActiveFocus READ controlHasActiveFocus
        WRITE setControlHasActiveFocus NOTIFY controlHasActiveFocusChanged FINAL)
    Q_PROPERTY(bool controlHasText READ controlHasText WRITE setControlHasText NOTIFY controlHasTextChanged FINAL)
    Q_PROPERTY(int largestHeight READ largestHeight NOTIFY largestHeightChanged FINAL)
    Q_PROPERTY(qreal verticalPadding READ verticalPadding WRITE setVerticalPadding NOTIFY verticalPaddingChanged FINAL)
    Q_PROPERTY(qreal controlImplicitBackgroundHeight READ controlImplicitBackgroundHeight
        WRITE setControlImplicitBackgroundHeight NOTIFY controlImplicitBackgroundHeightChanged FINAL)
    Q_PROPERTY(qreal controlHeight READ controlHeight WRITE setControlHeight FINAL)
    Q_PROPERTY(int leftPadding MEMBER m_leftPadding WRITE setLeftPadding FINAL)
    Q_PROPERTY(int floatingLeftPadding MEMBER m_floatingLeftPadding WRITE setFloatingLeftPadding FINAL)
    QML_NAMED_ELEMENT(FloatingPlaceholderText)
    QML_ADDED_IN_VERSION(6, 5)

public:
    explicit QQuickMaterialPlaceholderText(QQuickItem *parent = nullptr);

    bool isFilled() const;
    void setFilled(bool filled);

    int largestHeight() const;

    bool controlHasActiveFocus() const;
    void setControlHasActiveFocus(bool controlHasActiveFocus);

    bool controlHasText() const;
    void setControlHasText(bool controlHasText);

    qreal controlImplicitBackgroundHeight() const;
    void setControlImplicitBackgroundHeight(qreal controlImplicitBackgroundHeight);

    qreal controlHeight() const;
    void setControlHeight(qreal controlHeight);

    qreal verticalPadding() const;
    void setVerticalPadding(qreal verticalPadding);

    void setLeftPadding(int leftPadding);
    void setFloatingLeftPadding(int floatingLeftPadding);
signals:
    void filledChanged();
    void largestHeightChanged();
    void controlHasActiveFocusChanged();
    void controlHasTextChanged();
    void controlImplicitBackgroundHeightChanged();
    void verticalPaddingChanged();

private slots:
    void adjustTransformOrigin();

private:
    bool shouldFloat() const;
    bool shouldAnimate() const;

    void updateY();
    void updateX();
    qreal normalTargetY() const;
    qreal floatingTargetY() const;
    qreal normalTargetX() const;
    qreal floatingTargetX() const;

    void controlActiveFocusChanged();

    void updateFocusAnimation(bool createIfNeeded = false);

    void componentComplete() override;

    bool m_filled = false;
    bool m_controlHasActiveFocus = false;
    bool m_controlHasText = false;
    int m_largestHeight = 0;
    qreal m_verticalPadding = 0;
    qreal m_controlImplicitBackgroundHeight = 0;
    qreal m_controlHeight = 0;
    int m_leftPadding = 0;
    int m_floatingLeftPadding = 0;
    QPointer<QParallelAnimationGroup> m_focusAnimation;
};

QT_END_NAMESPACE

#endif // QQUICKMATERIALPLACEHOLDERTEXT_P_H
