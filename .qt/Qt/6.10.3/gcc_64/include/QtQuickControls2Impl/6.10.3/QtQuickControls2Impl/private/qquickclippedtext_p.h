// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCLIPPEDTEXT_P_H
#define QQUICKCLIPPEDTEXT_P_H

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

#include <QtQuick/private/qquicktext_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickClippedText : public QQuickText
{
    Q_OBJECT
    Q_PROPERTY(qreal clipX READ clipX WRITE setClipX FINAL)
    Q_PROPERTY(qreal clipY READ clipY WRITE setClipY FINAL)
    Q_PROPERTY(qreal clipWidth READ clipWidth WRITE setClipWidth FINAL)
    Q_PROPERTY(qreal clipHeight READ clipHeight WRITE setClipHeight FINAL)
    QML_NAMED_ELEMENT(ClippedText)
    QML_ADDED_IN_VERSION(2, 2)

public:
    explicit QQuickClippedText(QQuickItem *parent = nullptr);

    qreal clipX() const;
    void setClipX(qreal x);

    qreal clipY() const;
    void setClipY(qreal y);

    qreal clipWidth() const;
    void setClipWidth(qreal width);

    qreal clipHeight() const;
    void setClipHeight(qreal height);

    QRectF clipRect() const override;

private:
    void markClipDirty();

    bool m_hasClipWidth = false;
    bool m_hasClipHeight = false;
    qreal m_clipX = 0;
    qreal m_clipY = 0;
    qreal m_clipWidth = 0;
    qreal m_clipHeight = 0;
};

QT_END_NAMESPACE

#endif // QQUICKCLIPPEDTEXT_P_H
