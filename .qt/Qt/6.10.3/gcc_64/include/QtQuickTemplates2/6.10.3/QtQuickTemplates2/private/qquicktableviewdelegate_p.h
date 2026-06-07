// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTABLEVIEWDELEGATE_P_H
#define QQUICKTABLEVIEWDELEGATE_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickTableView;
class QQuickTableViewDelegatePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickTableViewDelegate : public QQuickItemDelegate
{
    Q_OBJECT

    // Required properties
    Q_PROPERTY(QQuickTableView *tableView READ tableView WRITE setTableView NOTIFY tableViewChanged REQUIRED FINAL)
    Q_PROPERTY(bool current READ current WRITE setCurrent NOTIFY currentChanged REQUIRED FINAL)
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged REQUIRED FINAL)
    Q_PROPERTY(bool editing READ editing WRITE setEditing NOTIFY editingChanged REQUIRED FINAL)

    QML_NAMED_ELEMENT(TableViewDelegate)
    QML_ADDED_IN_VERSION(6, 9)

public:
    explicit QQuickTableViewDelegate(QQuickItem *parent = nullptr);

    bool current() const;
    void setCurrent(bool current);

    bool selected() const;
    void setSelected(bool selected);

    bool editing() const;
    void setEditing(bool editing);

    QQuickTableView *tableView() const;
    void setTableView(QQuickTableView *tableView);

Q_SIGNALS:
    void tableViewChanged();
    void currentChanged();
    void selectedChanged();
    void editingChanged();

protected:
    QQuickTableViewDelegate(QQuickTableViewDelegatePrivate &dd, QQuickItem *parent);

    QFont defaultFont() const override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickTableViewDelegate)
    Q_DECLARE_PRIVATE(QQuickTableViewDelegate)
};

QT_END_NAMESPACE

#endif // QQUICKTABLEVIEWDELEGATE_P_H
