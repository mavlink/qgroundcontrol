// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTAREA_P_H
#define QQUICKTEXTAREA_P_H

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
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickText;
class QQuickTextAreaPrivate;
class QQuickTextAreaAttached;

class Q_QUICKTEMPLATES2_EXPORT QQuickTextArea : public QQuickTextEdit
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged) // override
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged3 FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged3 FINAL)
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged FINAL)
    Q_PROPERTY(Qt::FocusReason focusReason READ focusReason WRITE setFocusReason NOTIFY focusReasonChanged FINAL)
    // 2.1 (Qt 5.8)
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged FINAL REVISION(2, 1))
    Q_PROPERTY(bool hoverEnabled READ isHoverEnabled WRITE setHoverEnabled RESET resetHoverEnabled NOTIFY hoverEnabledChanged FINAL REVISION(2, 1))
    // 2.5 (Qt 5.12)
    Q_PROPERTY(QColor placeholderTextColor READ placeholderTextColor WRITE setPlaceholderTextColor NOTIFY placeholderTextColorChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitBackgroundWidth READ implicitBackgroundWidth NOTIFY implicitBackgroundWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitBackgroundHeight READ implicitBackgroundHeight NOTIFY implicitBackgroundHeightChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal topInset READ topInset WRITE setTopInset RESET resetTopInset NOTIFY topInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal leftInset READ leftInset WRITE setLeftInset RESET resetLeftInset NOTIFY leftInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal rightInset READ rightInset WRITE setRightInset RESET resetRightInset NOTIFY rightInsetChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal bottomInset READ bottomInset WRITE setBottomInset RESET resetBottomInset NOTIFY bottomInsetChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background")
    QML_NAMED_ELEMENT(TextArea)
    QML_ATTACHED(QQuickTextAreaAttached)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickTextArea(QQuickItem *parent = nullptr);
    ~QQuickTextArea();

    static QQuickTextAreaAttached *qmlAttachedProperties(QObject *object);

    QFont font() const;
    void setFont(const QFont &font);

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    QString placeholderText() const;
    void setPlaceholderText(const QString &text);

    Qt::FocusReason focusReason() const;
    void setFocusReason(Qt::FocusReason reason);

    bool contains(const QPointF &point) const override;

    // 2.1 (Qt 5.8)
    bool isHovered() const;
    void setHovered(bool hovered);

    bool isHoverEnabled() const;
    void setHoverEnabled(bool enabled);
    void resetHoverEnabled();

    // 2.5 (Qt 5.12)
    QColor placeholderTextColor() const;
    void setPlaceholderTextColor(const QColor &color);

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
    void implicitWidthChanged3();
    void implicitHeightChanged3();
    void backgroundChanged();
    void placeholderTextChanged();
    void focusReasonChanged();
    void pressAndHold(QQuickMouseEvent *event);
    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) void pressed(QQuickMouseEvent *event);
    Q_REVISION(2, 1) void released(QQuickMouseEvent *event);
    Q_REVISION(2, 1) void hoveredChanged();
    Q_REVISION(2, 1) void hoverEnabledChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void placeholderTextColorChanged();
    Q_REVISION(2, 5) void implicitBackgroundWidthChanged();
    Q_REVISION(2, 5) void implicitBackgroundHeightChanged();
    Q_REVISION(2, 5) void topInsetChanged();
    Q_REVISION(2, 5) void leftInsetChanged();
    Q_REVISION(2, 5) void rightInsetChanged();
    Q_REVISION(2, 5) void bottomInsetChanged();

protected:
    friend struct QQuickPressHandler;

    void classBegin() override;
    void componentComplete() override;

    void itemChange(ItemChange change, const ItemChangeData &value) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    virtual void insetChange(const QMarginsF &newInset, const QMarginsF &oldInset);

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
#if QT_CONFIG(quicktemplates2_hover)
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
#endif
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickTextArea)
    Q_DECLARE_PRIVATE(QQuickTextArea)
};

class QQuickTextAreaAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickTextAreaAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextArea *flickable READ flickable WRITE setFlickable NOTIFY flickableChanged FINAL)

public:
    explicit QQuickTextAreaAttached(QObject *parent);

    QQuickTextArea *flickable() const;
    void setFlickable(QQuickTextArea *control);

Q_SIGNALS:
    void flickableChanged();

private:
    Q_DISABLE_COPY(QQuickTextAreaAttached)
    Q_DECLARE_PRIVATE(QQuickTextAreaAttached)
};

QT_END_NAMESPACE

#endif // QQUICKTEXTAREA_P_H
