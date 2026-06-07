// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPANE_P_H
#define QQUICKPANE_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQml/qqmllist.h>

QT_BEGIN_NAMESPACE

class QQuickPanePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickPane : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth RESET resetContentWidth NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight RESET resetContentHeight NOTIFY contentHeightChanged FINAL)
    Q_PRIVATE_PROPERTY(QQuickPane::d_func(), QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PRIVATE_PROPERTY(QQuickPane::d_func(), QQmlListProperty<QQuickItem> contentChildren READ contentChildren NOTIFY contentChildrenChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "contentData")
    QML_NAMED_ELEMENT(Pane)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickPane(QQuickItem *parent = nullptr);
    ~QQuickPane();

    qreal contentWidth() const;
    void setContentWidth(qreal width);
    void resetContentWidth();

    qreal contentHeight() const;
    void setContentHeight(qreal height);
    void resetContentHeight();

Q_SIGNALS:
    void contentWidthChanged();
    void contentHeightChanged();
    void contentChildrenChanged();

protected:
    QQuickPane(QQuickPanePrivate &dd, QQuickItem *parent);

    void componentComplete() override;

    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    virtual void contentSizeChange(const QSizeF &newSize, const QSizeF &oldSize);

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickPane)
    Q_DECLARE_PRIVATE(QQuickPane)
};

QT_END_NAMESPACE

#endif // QQUICKPANE_P_H
