// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGROUPBOX_P_H
#define QQUICKGROUPBOX_P_H

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

#include <QtQuickTemplates2/private/qquickframe_p.h>

QT_BEGIN_NAMESPACE

class QQuickGroupBoxPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickGroupBox : public QQuickFrame
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQuickItem *label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitLabelWidth READ implicitLabelWidth NOTIFY implicitLabelWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitLabelHeight READ implicitLabelHeight NOTIFY implicitLabelHeightChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background,contentItem,label")
    QML_NAMED_ELEMENT(GroupBox)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickGroupBox(QQuickItem *parent = nullptr);
    ~QQuickGroupBox();

    QString title() const;
    void setTitle(const QString &title);

    QQuickItem *label() const;
    void setLabel(QQuickItem *label);

    // 2.5 (Qt 5.12)
    qreal implicitLabelWidth() const;
    qreal implicitLabelHeight() const;

Q_SIGNALS:
    void titleChanged();
    void labelChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void implicitLabelWidthChanged();
    Q_REVISION(2, 5) void implicitLabelHeightChanged();

protected:
    void componentComplete() override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
    void accessibilityActiveChanged(bool active) override;
#endif

private:
    Q_DISABLE_COPY(QQuickGroupBox)
    Q_DECLARE_PRIVATE(QQuickGroupBox)
};

QT_END_NAMESPACE

#endif // QQUICKGROUPBOX_P_H
