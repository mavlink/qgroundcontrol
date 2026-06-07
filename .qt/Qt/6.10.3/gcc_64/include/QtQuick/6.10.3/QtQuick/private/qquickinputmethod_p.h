// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKINPUTMETHOD_P_H
#define QQUICKINPUTMETHOD_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qlocale.h>
#include <QtCore/qrect.h>
#include <QtGui/qtransform.h>
#include <QtGui/qinputmethod.h>
#include <QtQml/qqml.h>

#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE
class Q_QUICK_EXPORT QQuickInputMethod : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InputMethod)
    QML_ADDED_IN_VERSION(6, 4)
    QML_SINGLETON

    Q_PROPERTY(QRectF cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged FINAL)
    Q_PROPERTY(QRectF anchorRectangle READ anchorRectangle NOTIFY anchorRectangleChanged FINAL)
    Q_PROPERTY(QRectF keyboardRectangle READ keyboardRectangle NOTIFY keyboardRectangleChanged FINAL)
    Q_PROPERTY(QRectF inputItemClipRectangle READ inputItemClipRectangle NOTIFY
                       inputItemClipRectangleChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool animating READ isAnimating NOTIFY animatingChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale NOTIFY localeChanged FINAL)
    Q_PROPERTY(Qt::LayoutDirection inputDirection READ inputDirection NOTIFY inputDirectionChanged FINAL)
public:
    explicit QQuickInputMethod(QObject *parent = nullptr);

    QRectF anchorRectangle() const;
    QRectF cursorRectangle() const;
    Qt::LayoutDirection inputDirection() const;
    QRectF inputItemClipRectangle() const;

    QRectF inputItemRectangle() const;
    void setInputItemRectangle(const QRectF &rect);

    QTransform inputItemTransform() const;
    void setInputItemTransform(const QTransform &transform);

    bool isAnimating() const;

    bool isVisible() const;
    void setVisible(bool visible);

    QRectF keyboardRectangle() const;
    QLocale locale() const;
Q_SIGNALS:
    void anchorRectangleChanged();
    void animatingChanged();
    void cursorRectangleChanged();
    void inputDirectionChanged(Qt::LayoutDirection newDirection);
    void inputItemClipRectangleChanged();
    void keyboardRectangleChanged();
    void localeChanged();
    void visibleChanged();

public Q_SLOTS:
    void commit();
    void hide();
    void invokeAction(QInputMethod::Action a, int cursorPosition);
    void reset();
    void show();
    void update(Qt::InputMethodQueries queries);
};

QT_END_NAMESPACE

#endif // QQUICKINPUTMETHOD_P_H
