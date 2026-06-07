// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKBUTTON_P_H
#define QQUICKBUTTON_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickButtonPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickButton : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat NOTIFY flatChanged FINAL)
    QML_NAMED_ELEMENT(Button)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickButton(QQuickItem *parent = nullptr);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

    bool isFlat() const;
    void setFlat(bool flat);

Q_SIGNALS:
    void highlightedChanged();
    void flatChanged();

protected:
    QQuickButton(QQuickButtonPrivate &dd, QQuickItem *parent);

    QFont defaultFont() const override;

private:
    Q_DISABLE_COPY(QQuickButton)
    Q_DECLARE_PRIVATE(QQuickButton)
};

QT_END_NAMESPACE

#endif // QQUICKBUTTON_P_H
