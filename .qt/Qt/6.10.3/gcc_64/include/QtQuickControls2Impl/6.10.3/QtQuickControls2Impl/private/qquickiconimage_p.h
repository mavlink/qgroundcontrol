// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKICONIMAGE_P_H
#define QQUICKICONIMAGE_P_H

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
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickIconImagePrivate;

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickIconImage : public QQuickImage
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    QML_NAMED_ELEMENT(IconImage)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickIconImage(QQuickItem *parent = nullptr);

    QString name() const;
    void setName(const QString &name);

    QColor color() const;
    void setColor(const QColor &color);

    void setSource(const QUrl &url) override;

    void snapPositionTo(QPointF pos);

Q_SIGNALS:
    void nameChanged();
    void colorChanged();

protected:
    void componentComplete() override;
    void load() override;

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void pixmapChange() override;

private:
    Q_DISABLE_COPY(QQuickIconImage)
    Q_DECLARE_PRIVATE(QQuickIconImage)
};

QT_END_NAMESPACE

#endif // QQUICKICONIMAGE_P_H
