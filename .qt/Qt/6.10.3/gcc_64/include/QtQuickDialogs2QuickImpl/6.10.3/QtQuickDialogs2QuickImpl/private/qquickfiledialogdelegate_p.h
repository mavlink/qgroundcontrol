// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFILEDIALOGDELEGATE_P_H
#define QQUICKFILEDIALOGDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickDialog;
class QQuickFileDialogDelegatePrivate;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickFileDialogDelegate : public QQuickItemDelegate
{
    Q_OBJECT
    Q_PROPERTY(QQuickDialog *dialog READ dialog WRITE setDialog NOTIFY dialogChanged)
    Q_PROPERTY(QUrl file READ file WRITE setFile NOTIFY fileChanged)
    QML_NAMED_ELEMENT(FileDialogDelegate)
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickFileDialogDelegate(QQuickItem *parent = nullptr);

    QQuickDialog *dialog() const;
    void setDialog(QQuickDialog *dialog);

    QUrl file() const;
    void setFile(const QUrl &file);

Q_SIGNALS:
    void dialogChanged();
    void fileChanged();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickFileDialogDelegate)
    Q_DECLARE_PRIVATE(QQuickFileDialogDelegate)
};

QT_END_NAMESPACE

#endif // QQUICKFILEDIALOGDELEGATE_P_H
