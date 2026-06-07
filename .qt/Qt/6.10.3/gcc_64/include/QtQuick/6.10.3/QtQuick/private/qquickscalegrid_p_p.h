// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSCALEGRID_P_P_H
#define QQUICKSCALEGRID_P_P_H

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

#include "qquickborderimage_p.h"

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

#include <QtQuick/private/qquickpixmap_p.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickScaleGrid : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int left READ left WRITE setLeft NOTIFY leftBorderChanged FINAL)
    Q_PROPERTY(int top READ top WRITE setTop NOTIFY topBorderChanged FINAL)
    Q_PROPERTY(int right READ right WRITE setRight NOTIFY rightBorderChanged FINAL)
    Q_PROPERTY(int bottom READ bottom WRITE setBottom NOTIFY bottomBorderChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickScaleGrid(QObject *parent=nullptr);

    bool isNull() const;

    int left() const { return _left; }
    void setLeft(int);

    int top() const { return _top; }
    void setTop(int);

    int right() const { return _right; }
    void setRight(int);

    int  bottom() const { return _bottom; }
    void setBottom(int);

Q_SIGNALS:
    void borderChanged();
    void leftBorderChanged();
    void topBorderChanged();
    void rightBorderChanged();
    void bottomBorderChanged();

private:
    int _left;
    int _top;
    int _right;
    int _bottom;
};

class Q_AUTOTEST_EXPORT QQuickGridScaledImage
{
public:
    QQuickGridScaledImage();
    QQuickGridScaledImage(const QQuickGridScaledImage &);
    QQuickGridScaledImage(QIODevice*);
    QQuickGridScaledImage &operator=(const QQuickGridScaledImage &);
    bool isValid() const;
    int gridLeft() const;
    int gridRight() const;
    int gridTop() const;
    int gridBottom() const;
    QQuickBorderImage::TileMode horizontalTileRule() const { return _h; }
    QQuickBorderImage::TileMode verticalTileRule() const { return _v; }

    QString pixmapUrl() const;

private:
    static QQuickBorderImage::TileMode stringToRule(QStringView);

private:
    int _l;
    int _r;
    int _t;
    int _b;
    QQuickBorderImage::TileMode _h;
    QQuickBorderImage::TileMode _v;
    QString _pix;
};

QT_END_NAMESPACE

#endif // QQUICKSCALEGRID_P_P_H
