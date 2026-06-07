// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSPLITVIEW_P_H
#define QQUICKSPLITVIEW_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p.h>
#include <QtQml/qqmllist.h>

QT_REQUIRE_CONFIG(quicktemplates2_container);

QT_BEGIN_NAMESPACE

class QQuickSplitViewPrivate;
class QQuickSplitViewAttached;
class QQuickSplitViewAttachedPrivate;
class QQuickSplitHandleAttached;
class QQuickSplitHandleAttachedPrivate;
class QQmlComponent;

class Q_QUICKTEMPLATES2_EXPORT QQuickSplitView : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool resizing READ isResizing NOTIFY resizingChanged)
    Q_PROPERTY(QQmlComponent *handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    QML_NAMED_ELEMENT(SplitView)
    QML_ATTACHED(QQuickSplitViewAttached)
    QML_ADDED_IN_VERSION(2, 13)

public:
    explicit QQuickSplitView(QQuickItem *parent = nullptr);
    ~QQuickSplitView() override;

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    bool isResizing() const;

    QQmlComponent *handle();
    void setHandle(QQmlComponent *handle);

    bool isContent(QQuickItem *item) const override;

    static QQuickSplitViewAttached *qmlAttachedProperties(QObject *object);

    // Based on the same code in QMainWindow.
    enum VersionMarkers {
        VersionMarker = 0xff
    };
    Q_INVOKABLE QVariant saveState();
    Q_INVOKABLE bool restoreState(const QVariant &state);

Q_SIGNALS:
    void orientationChanged();
    void resizingChanged();
    void handleChanged();

protected:
    QQuickSplitView(QQuickSplitViewPrivate &dd, QQuickItem *parent);

    void componentComplete() override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void itemAdded(int index, QQuickItem *item) override;
    void itemMoved(int index, QQuickItem *item) override;
    void itemRemoved(int index, QQuickItem *item) override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickSplitView)
    Q_DECLARE_PRIVATE(QQuickSplitView)
};

class Q_QUICKTEMPLATES2_EXPORT QQuickSplitViewAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickSplitView *view READ view NOTIFY viewChanged FINAL)
    Q_PROPERTY(qreal minimumWidth READ minimumWidth WRITE setMinimumWidth
        RESET resetMinimumWidth NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(qreal minimumHeight READ minimumHeight WRITE setMinimumHeight
        RESET resetMinimumHeight NOTIFY minimumHeightChanged FINAL)
    Q_PROPERTY(qreal preferredWidth READ preferredWidth WRITE setPreferredWidth
        RESET resetPreferredWidth NOTIFY preferredWidthChanged FINAL)
    Q_PROPERTY(qreal preferredHeight READ preferredHeight WRITE setPreferredHeight
        RESET resetPreferredHeight NOTIFY preferredHeightChanged FINAL)
    Q_PROPERTY(qreal maximumWidth READ maximumWidth WRITE setMaximumWidth
        RESET resetMaximumWidth NOTIFY maximumWidthChanged FINAL)
    Q_PROPERTY(qreal maximumHeight READ maximumHeight WRITE setMaximumHeight
        RESET resetMaximumHeight NOTIFY maximumHeightChanged FINAL)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight NOTIFY fillHeightChanged FINAL)
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged FINAL)

public:
    explicit QQuickSplitViewAttached(QObject *parent = nullptr);

    QQuickSplitView *view() const;

    qreal minimumWidth() const;
    void setMinimumWidth(qreal width);
    void resetMinimumWidth();

    qreal minimumHeight() const;
    void setMinimumHeight(qreal height);
    void resetMinimumHeight();

    qreal preferredWidth() const;
    void setPreferredWidth(qreal width);
    void resetPreferredWidth();

    qreal preferredHeight() const;
    void setPreferredHeight(qreal height);
    void resetPreferredHeight();

    qreal maximumWidth() const;
    void setMaximumWidth(qreal width);
    void resetMaximumWidth();

    qreal maximumHeight() const;
    void setMaximumHeight(qreal height);
    void resetMaximumHeight();

    bool fillWidth() const;
    void setFillWidth(bool fill);

    bool fillHeight() const;
    void setFillHeight(bool fill);

Q_SIGNALS:
    void viewChanged();
    void minimumWidthChanged();
    void minimumHeightChanged();
    void preferredWidthChanged();
    void preferredHeightChanged();
    void maximumWidthChanged();
    void maximumHeightChanged();
    void fillWidthChanged();
    void fillHeightChanged();

private:
    Q_DISABLE_COPY(QQuickSplitViewAttached)
    Q_DECLARE_PRIVATE(QQuickSplitViewAttached)
};

class Q_QUICKTEMPLATES2_EXPORT QQuickSplitHandleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)
    QML_NAMED_ELEMENT(SplitHandle)
    QML_ATTACHED(QQuickSplitHandleAttached)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 13)

public:
    explicit QQuickSplitHandleAttached(QObject *parent = nullptr);

    bool isHovered() const;
    bool isPressed() const;

    static QQuickSplitHandleAttached *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void hoveredChanged();
    void pressedChanged();

private:
    Q_DISABLE_COPY(QQuickSplitHandleAttached)
    Q_DECLARE_PRIVATE(QQuickSplitHandleAttached)
};

QT_END_NAMESPACE

#endif // QQUICKSPLITVIEW_P_H
