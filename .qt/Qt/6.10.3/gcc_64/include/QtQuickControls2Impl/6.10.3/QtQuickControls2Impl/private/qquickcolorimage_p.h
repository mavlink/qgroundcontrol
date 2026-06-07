// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCOLORIMAGE_P_H
#define QQUICKCOLORIMAGE_P_H

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
#include <QtQuick/private/qquickimage_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickColorImage : public QQuickImage
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor RESET resetColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QColor defaultColor READ defaultColor WRITE setDefaultColor RESET resetDefaultColor NOTIFY defaultColorChanged FINAL)
    QML_NAMED_ELEMENT(ColorImage)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickColorImage(QQuickItem *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);
    void resetColor();

    QColor defaultColor() const;
    void setDefaultColor(const QColor &color);
    void resetDefaultColor();

Q_SIGNALS:
    void colorChanged();
    void defaultColorChanged();

protected:
    void pixmapChange() override;

private:
    QColor m_color = Qt::transparent;
    QColor m_defaultColor = Qt::transparent;
};

QT_END_NAMESPACE

#endif // QQUICKCOLORIMAGE_P_H
