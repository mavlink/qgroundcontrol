// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTACKELEMENT_P_P_H
#define QQUICKSTACKELEMENT_P_P_H

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
#include <QtQml/private/qv4persistent_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlComponent;
struct QQuickStackTransition;
class RequiredProperties;

class QQuickStackElement :
#if QT_CONFIG(quick_viewtransitions)
        public QQuickItemViewTransitionableItem,
#endif
        public QQuickItemChangeListener
{
    QQuickStackElement();

public:
    ~QQuickStackElement();

    static QQuickStackElement *fromString(QQmlEngine *engine, const QString &str, QQuickStackView *view, QString *error);
    static QQuickStackElement *fromObject(QObject *object, QQuickStackView *view, QString *error);
    static QQuickStackElement *fromStackViewArg(QQmlEngine *engine, QQuickStackView *view, QQuickStackViewArg arg);

    bool load(QV4::ExecutionEngine *v4, QQuickStackView *parent);
    void incubate(
            QV4::ExecutionEngine *v4, QObject *object, RequiredProperties *requiredProperties);
    void initialize(QV4::ExecutionEngine *v4, RequiredProperties *requiredProperties);

    void setIndex(int index);
    void setView(QQuickStackView *view);
    void setStatus(QQuickStackView::Status status);
    void setVisible(bool visible);

#if QT_CONFIG(quick_viewtransitions)
    void transitionNextReposition(QQuickItemViewTransitioner *transitioner, QQuickItemViewTransitioner::TransitionType type, bool asTarget);
    bool prepareTransition(QQuickItemViewTransitioner *transitioner, const QRectF &viewBounds);
    void startTransition(QQuickItemViewTransitioner *transitioner, QQuickStackView::Status status);
    void completeTransition(QQuickTransition *quickTransition);
#endif

    void itemDestroyed(QQuickItem *item) override;

    int index = -1;
    bool init = false;
    bool removal = false;
    bool ownItem = false;
    bool ownComponent = false;
    bool widthValid = false;
    bool heightValid = false;
    QQmlComponent *component = nullptr;
    QQuickStackView *view = nullptr;
    QPointer<QQuickItem> originalParent;
    QQuickStackView::Status status = QQuickStackView::Inactive;
    QV4::PersistentValue properties;
    QV4::PersistentValue qmlCallingContext;
#if !QT_CONFIG(quick_viewtransitions)
    QQuickItem *item;
#endif
};

QT_END_NAMESPACE

#endif // QQUICKSTACKELEMENT_P_P_H
