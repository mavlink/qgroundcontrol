// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKHEADERVIEWDELEGATE_P_H
#define QQUICKHEADERVIEWDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquicktableviewdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickHeaderViewBase;
class QQuickHeaderViewDelegatePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickHeaderViewDelegate : public QQuickTableViewDelegate
{
    Q_OBJECT

    Q_PROPERTY(Qt::Orientation orientation READ orientation NOTIFY orientationChanged FINAL)
    // Required properties
    Q_PROPERTY(QQuickHeaderViewBase *headerView READ headerView WRITE setHeaderView NOTIFY headerViewChanged REQUIRED FINAL)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged REQUIRED FINAL)

    QML_NAMED_ELEMENT(HeaderViewDelegate)
    QML_ADDED_IN_VERSION(6, 10)

public:
    QQuickHeaderViewDelegate(QQuickItem *parent = nullptr);

    QQuickHeaderViewBase *headerView() const;
    void setHeaderView(QQuickHeaderViewBase *headerView);

    Qt::Orientation orientation() const;

    QVariant model() const;
    void setModel(const QVariant &model);

Q_SIGNALS:
    void headerViewChanged();
    void modelChanged();
    void orientationChanged();

private:
    Q_DISABLE_COPY(QQuickHeaderViewDelegate)
    Q_DECLARE_PRIVATE(QQuickHeaderViewDelegate)
};

QT_END_NAMESPACE

#endif // QQUICKHEADERVIEWDELEGATE_P_H
