// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKNINEPATCHIMAGE_P_H
#define QQUICKNINEPATCHIMAGE_P_H

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

#include <QtQuick/private/qquickimage_p.h>

QT_BEGIN_NAMESPACE

class QQuickNinePatchImagePrivate;

class QQuickNinePatchImage :  public QQuickImage
{
    Q_OBJECT
    Q_PROPERTY(qreal topPadding READ topPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding NOTIFY bottomPaddingChanged FINAL)
    Q_PROPERTY(qreal topInset READ topInset NOTIFY topInsetChanged FINAL)
    Q_PROPERTY(qreal leftInset READ leftInset NOTIFY leftInsetChanged FINAL)
    Q_PROPERTY(qreal rightInset READ rightInset NOTIFY rightInsetChanged FINAL)
    Q_PROPERTY(qreal bottomInset READ bottomInset NOTIFY bottomInsetChanged FINAL)
    QML_NAMED_ELEMENT(NinePatchImage)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickNinePatchImage(QQuickItem *parent = nullptr);

    qreal topPadding() const;
    qreal leftPadding() const;
    qreal rightPadding() const;
    qreal bottomPadding() const;

    qreal topInset() const;
    qreal leftInset() const;
    qreal rightInset() const;
    qreal bottomInset() const;

Q_SIGNALS:
    void topPaddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void bottomPaddingChanged();

    void topInsetChanged();
    void leftInsetChanged();
    void rightInsetChanged();
    void bottomInsetChanged();

protected:
    void pixmapChange() override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    Q_DISABLE_COPY(QQuickNinePatchImage)
    Q_DECLARE_PRIVATE(QQuickNinePatchImage)
};

QT_END_NAMESPACE

#endif // QQUICKNINEPATCHIMAGE_P_H
