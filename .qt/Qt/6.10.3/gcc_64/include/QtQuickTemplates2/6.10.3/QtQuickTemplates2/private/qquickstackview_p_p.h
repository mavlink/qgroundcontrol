// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTACKVIEW_P_P_H
#define QQUICKSTACKVIEW_P_P_H

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

#include <QtQuickTemplates2/private/qquickstackview_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#if QT_CONFIG(quick_viewtransitions)
#include <QtQuick/private/qquickitemviewtransition_p.h>
#endif
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQml/private/qv4value_p.h>
#include <QtQml/private/qqmlcontextdata_p.h>
#include <QtCore/qset.h>
#include <QtCore/qstack.h>

QT_BEGIN_NAMESPACE

class QQuickStackElement;
struct QQuickStackTransition;

class QQuickStackViewPrivate : public QQuickControlPrivate
#if QT_CONFIG(quick_viewtransitions)
        , public QQuickItemViewTransitionChangeListener
#endif
{
    Q_DECLARE_PUBLIC(QQuickStackView)

public:
    static QQuickStackViewPrivate *get(QQuickStackView *view)
    {
        return view->d_func();
    }

    void warn(const QString &error);
    void warnOfInterruption(const QString &attemptedOperation);

    void setCurrentItem(QQuickStackElement *element);

    QList<QQuickStackElement *> parseElements(int from, QQmlV4FunctionPtr args, QStringList *errors);
    QList<QQuickStackElement *> parseElements(QQmlEngine *engine, const QList<QQuickStackViewArg> &args);
    QQuickStackElement *findElement(QQuickItem *item) const;
    QQuickStackElement *findElement(const QV4::Value &value) const;
    QQuickStackElement *createElement(const QV4::Value &value, const QQmlRefPointer<QQmlContextData> &context, QString *error);
    bool pushElements(QV4::ExecutionEngine *v4, const QList<QQuickStackElement *> &elements);
    bool pushElement(QV4::ExecutionEngine *v4, QQuickStackElement *element);
    bool popElements(QV4::ExecutionEngine *v4, QQuickStackElement *element);
    bool replaceElements(QV4::ExecutionEngine *v4, QQuickStackElement *element, const QList<QQuickStackElement *> &elements);

    enum class CurrentItemPolicy {
        DoNotPop,
        Pop
    };
    QQuickItem *popToItem(QV4::ExecutionEngine *v4, QQuickItem *item, QQuickStackView::Operation operation, CurrentItemPolicy currentItemPolicy);

#if QT_CONFIG(quick_viewtransitions)
    void ensureTransitioner();
    void startTransition(const QQuickStackTransition &first, const QQuickStackTransition &second, bool immediate);
    void completeTransition(QQuickStackElement *element, QQuickTransition *transition, QQuickStackView::Status status);

    void viewItemTransitionFinished(QQuickItemViewTransitionableItem *item) override;
#endif
    void setBusy(bool busy);
    void depthChange(int newDepth, int oldDepth);

    bool busy = false;
    bool modifyingElements = false;
    QString operation;
    QJSValue initialItem;
    QQuickItem *currentItem = nullptr;
    QSet<QQuickStackElement*> removing;
    QList<QQuickStackElement*> removed;
    QStack<QQuickStackElement *> elements;
#if QT_CONFIG(quick_viewtransitions)
    QQuickItemViewTransitioner *transitioner = nullptr;
#endif
};

class QQuickStackViewAttachedPrivate : public QObjectPrivate
//#if QT_CONFIG(quick_viewtransitions)
        , public QSafeQuickItemChangeListener<QQuickStackViewAttachedPrivate>
//#endif
{
    Q_DECLARE_PUBLIC(QQuickStackViewAttached)

public:
    static QQuickStackViewAttachedPrivate *get(QQuickStackViewAttached *attached)
    {
        return attached->d_func();
    }

//#if QT_CONFIG(quick_viewtransitions)
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) override;
//#endif

    bool explicitVisible = false;
    QQuickStackElement *element = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSTACKVIEW_P_P_H
