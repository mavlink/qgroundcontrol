// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABEL_P_H
#define QQUICKLABEL_P_H

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

#include <QtGui/qpalette.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickLabelPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickLabel : public QQuickText
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged) // override
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitBackgroundWidth READ implicitBackgroundWidth NOTIFY implicitBackgroundWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitBackgroundHeight READ implicitBackgroundHeight NOTIFY implicitBackgroundHeightChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal topInset READ topInset WRITE setTopInset RESET resetTopInset NOTIFY topInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal leftInset READ leftInset WRITE setLeftInset RESET resetLeftInset NOTIFY leftInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal rightInset READ rightInset WRITE setRightInset RESET resetRightInset NOTIFY rightInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal bottomInset READ bottomInset WRITE setBottomInset RESET resetBottomInset NOTIFY bottomInsetChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background")
    QML_NAMED_ELEMENT(Label)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickLabel(QQuickItem *parent = nullptr);
    ~QQuickLabel();

    QFont font() const;
    void setFont(const QFont &font);

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    // 2.5 (Qt 5.12)
    qreal implicitBackgroundWidth() const;
    qreal implicitBackgroundHeight() const;

    qreal topInset() const;
    void setTopInset(qreal inset);
    void resetTopInset();

    qreal leftInset() const;
    void setLeftInset(qreal inset);
    void resetLeftInset();

    qreal rightInset() const;
    void setRightInset(qreal inset);
    void resetRightInset();

    qreal bottomInset() const;
    void setBottomInset(qreal inset);
    void resetBottomInset();

Q_SIGNALS:
    void fontChanged();
    void backgroundChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void implicitBackgroundWidthChanged();
    Q_REVISION(2, 5) void implicitBackgroundHeightChanged();
    Q_REVISION(2, 5) void topInsetChanged();
    Q_REVISION(2, 5) void leftInsetChanged();
    Q_REVISION(2, 5) void rightInsetChanged();
    Q_REVISION(2, 5) void bottomInsetChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    virtual void insetChange(const QMarginsF &newInset, const QMarginsF &oldInset);

private:
    Q_DISABLE_COPY(QQuickLabel)
    Q_DECLARE_PRIVATE(QQuickLabel)
};

QT_END_NAMESPACE

#endif // QQUICKLABEL_P_H
