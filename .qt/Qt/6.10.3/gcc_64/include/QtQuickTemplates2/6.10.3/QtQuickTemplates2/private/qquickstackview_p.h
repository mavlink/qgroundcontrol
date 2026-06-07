// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTACKVIEW_P_H
#define QQUICKSTACKVIEW_P_H

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

#include <QtCore/qdebug.h>
#include <QtCore/qvariantmap.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickTransition;
class QQuickStackElement;
class QQuickStackViewPrivate;
class QQuickStackViewAttached;
class QQuickStackViewAttachedPrivate;
class QQmlComponent;

/*!
    \internal

    Input from the user that is turned into QQuickStackElements.

    This was added to support the QML-compiler-friendly pushElement[s]
    functions.
*/
class QQuickStackViewArg
{
    Q_GADGET
    QML_CONSTRUCTIBLE_VALUE
    QML_ANONYMOUS

public:
    QQuickStackViewArg() = default;
    Q_INVOKABLE QQuickStackViewArg(QQuickItem *item);
    Q_INVOKABLE QQuickStackViewArg(const QUrl &url);
    Q_INVOKABLE QQuickStackViewArg(QQmlComponent *component);
    Q_INVOKABLE QQuickStackViewArg(const QVariantMap &properties);

#ifndef QT_NO_DEBUG_STREAM
    friend QDebug operator<<(QDebug debug, const QQuickStackViewArg &arg);
#endif

private:
    friend class QQuickStackViewPrivate;
    friend class QQuickStackElement;

    QQuickItem *mItem = nullptr;
    QQmlComponent *mComponent = nullptr;
    QUrl mUrl;
    QVariantMap mProperties;
};

class Q_QUICKTEMPLATES2_EXPORT QQuickStackView : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged FINAL)
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged FINAL)
    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged FINAL)
    Q_PROPERTY(QJSValue initialItem READ initialItem WRITE setInitialItem FINAL)
#if QT_CONFIG(quick_viewtransitions)
    Q_PROPERTY(QQuickTransition *popEnter READ popEnter WRITE setPopEnter NOTIFY popEnterChanged FINAL)
    Q_PROPERTY(QQuickTransition *popExit READ popExit WRITE setPopExit NOTIFY popExitChanged FINAL)
    Q_PROPERTY(QQuickTransition *pushEnter READ pushEnter WRITE setPushEnter NOTIFY pushEnterChanged FINAL)
    Q_PROPERTY(QQuickTransition *pushExit READ pushExit WRITE setPushExit NOTIFY pushExitChanged FINAL)
    Q_PROPERTY(QQuickTransition *replaceEnter READ replaceEnter WRITE setReplaceEnter NOTIFY replaceEnterChanged FINAL)
    Q_PROPERTY(QQuickTransition *replaceExit READ replaceExit WRITE setReplaceExit NOTIFY replaceExitChanged FINAL)
#endif
    // 2.3 (Qt 5.10)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged FINAL REVISION(2, 3))
    QML_NAMED_ELEMENT(StackView)
    QML_ATTACHED(QQuickStackViewAttached)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickStackView(QQuickItem *parent = nullptr);
    ~QQuickStackView();

    static QQuickStackViewAttached *qmlAttachedProperties(QObject *object);

    bool isBusy() const;
    int depth() const;
    QQuickItem *currentItem() const;

    enum Status {
        Inactive = 0,
        Deactivating = 1,
        Activating = 2,
        Active = 3
    };
    Q_ENUM(Status)

    QJSValue initialItem() const;
    void setInitialItem(const QJSValue &item);

#if QT_CONFIG(quick_viewtransitions)
    QQuickTransition *popEnter() const;
    void setPopEnter(QQuickTransition *enter);

    QQuickTransition *popExit() const;
    void setPopExit(QQuickTransition *exit);

    QQuickTransition *pushEnter() const;
    void setPushEnter(QQuickTransition *enter);

    QQuickTransition *pushExit() const;
    void setPushExit(QQuickTransition *exit);

    QQuickTransition *replaceEnter() const;
    void setReplaceEnter(QQuickTransition *enter);

    QQuickTransition *replaceExit() const;
    void setReplaceExit(QQuickTransition *exit);
#endif

    enum LoadBehavior {
        DontLoad,
        ForceLoad
    };
    Q_ENUM(LoadBehavior)

    Q_INVOKABLE QQuickItem *get(int index, QQuickStackView::LoadBehavior behavior = DontLoad);
    Q_INVOKABLE QQuickItem *find(const QJSValue &callback, QQuickStackView::LoadBehavior behavior = DontLoad);

    enum Operation {
        Transition = -1, // ### Deprecated in Qt 6; remove in Qt 7.
        Immediate = 0,
        PushTransition = 1,
        ReplaceTransition = 2,
        PopTransition = 3,
    };
    Q_ENUM(Operation)

    Q_INVOKABLE void push(QQmlV4FunctionPtr args);
    Q_INVOKABLE void pop(QQmlV4FunctionPtr args);
    Q_INVOKABLE void replace(QQmlV4FunctionPtr args);

    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *pushItems(QList<QQuickStackViewArg> args,
        Operation operation = PushTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *pushItem(QQuickItem *item, const QVariantMap &properties = {},
        Operation operation = PushTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *pushItem(QQmlComponent *component, const QVariantMap &properties = {},
        Operation operation = PushTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *pushItem(const QUrl &url, const QVariantMap &properties = {},
        Operation operation = PushTransition);

    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *popToItem(QQuickItem *item, Operation operation = PopTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *popToIndex(int index, Operation operation = PopTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *popCurrentItem(Operation operation = PopTransition);

    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *replaceCurrentItem(const QList<QQuickStackViewArg> &args,
        Operation operation = ReplaceTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *replaceCurrentItem(QQuickItem *item,
        const QVariantMap &properties = {}, Operation operation = ReplaceTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *replaceCurrentItem(QQmlComponent *component,
        const QVariantMap &properties = {}, Operation operation = ReplaceTransition);
    Q_REVISION(6, 7) Q_INVOKABLE QQuickItem *replaceCurrentItem(const QUrl &url,
        const QVariantMap &properties = {}, Operation operation = ReplaceTransition);

    // 2.3 (Qt 5.10)
    bool isEmpty() const;

public Q_SLOTS:
    void clear(Operation operation = Immediate);

Q_SIGNALS:
    void busyChanged();
    void depthChanged();
    void currentItemChanged();
#if QT_CONFIG(quick_viewtransitions)
    void popEnterChanged();
    void popExitChanged();
    void pushEnterChanged();
    void pushExitChanged();
    void replaceEnterChanged();
    void replaceExitChanged();
#endif
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void emptyChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    bool childMouseEventFilter(QQuickItem *, QEvent *) override;

#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickStackView)
    Q_DECLARE_PRIVATE(QQuickStackView)
};

class Q_QUICKTEMPLATES2_EXPORT QQuickStackViewAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(QQuickStackView *view READ view NOTIFY viewChanged FINAL)
    Q_PROPERTY(QQuickStackView::Status status READ status NOTIFY statusChanged FINAL)
    // 2.2 (Qt 5.9)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible RESET resetVisible NOTIFY visibleChanged FINAL) // REVISION(2, 2)

public:
    explicit QQuickStackViewAttached(QObject *parent = nullptr);
    ~QQuickStackViewAttached();

    int index() const;
    QQuickStackView *view() const;
    QQuickStackView::Status status() const;

    // 2.2 (Qt 5.9)
    bool isVisible() const;
    void setVisible(bool visible);
    void resetVisible();

Q_SIGNALS:
    void indexChanged();
    void viewChanged();
    void statusChanged();
    // 2.1 (Qt 5.8)
    /*Q_REVISION(2, 1)*/ void activated();
    /*Q_REVISION(2, 1)*/ void activating();
    /*Q_REVISION(2, 1)*/ void deactivated();
    /*Q_REVISION(2, 1)*/ void deactivating();
    /*Q_REVISION(2, 1)*/ void removed();
    // 2.2 (Qt 5.9)
    /*Q_REVISION(2, 2)*/ void visibleChanged();

private:
    Q_DISABLE_COPY(QQuickStackViewAttached)
    Q_DECLARE_PRIVATE(QQuickStackViewAttached)
};

QT_END_NAMESPACE

#endif // QQUICKSTACKVIEW_P_H
