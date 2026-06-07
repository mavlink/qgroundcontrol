// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINCUBATOR_P_H
#define QQMLINCUBATOR_P_H

#include "qqmlincubator.h"

#include <private/qintrusivelist_p.h>
#include <private/qqmlvme_p.h>
#include <private/qrecursionwatcher_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlguardedcontextdata_p.h>

#include <QtCore/qpointer.h>

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

QT_BEGIN_NAMESPACE

class RequiredProperties;

class QQmlIncubator;
class Q_QML_EXPORT QQmlIncubatorPrivate : public QQmlEnginePrivate::Incubator, public QSharedData
{
public:
    QQmlIncubatorPrivate(QQmlIncubator *q, QQmlIncubator::IncubationMode m);
    ~QQmlIncubatorPrivate();

    inline static QQmlIncubatorPrivate *get(QQmlIncubator *incubator) { return incubator->d; }

    int subComponentToCreate;
    QQmlIncubator *q;

    QQmlIncubator::Status calculateStatus() const;
    void changeStatus(QQmlIncubator::Status);
    QQmlIncubator::Status status;

    QQmlIncubator::IncubationMode mode;
    bool isAsynchronous;
    enum Progress : char { Execute, Completing, Completed };
    Progress progress;

    QList<QQmlError> errors;


    QPointer<QObject> result;
    enum HadTopLevelRequired : bool {No = 0, Yes = 1};
    /* TODO: unify with Creator pointer once QTBUG-108760 is implemented
       though we don't acutally own the properties here; if we ever end up
       with a use case for async incubation of C++ types, we however could
       not rely on the component to still exist during incubation, and
       would need to store a copy of the required properties instead
    */
    QTaggedPointer<RequiredProperties, HadTopLevelRequired> requiredPropertiesFromComponent;
    QQmlGuardedContextData rootContext;
    QQmlEnginePrivate *enginePriv;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QScopedPointer<QQmlObjectCreator> creator;
    QQmlVMEGuard vmeGuard;

    QExplicitlySharedDataPointer<QQmlIncubatorPrivate> waitingOnMe;
    typedef QQmlEnginePrivate::Incubator QIPBase;
    QIntrusiveListNode nextWaitingFor;
    QIntrusiveList<QQmlIncubatorPrivate, &QQmlIncubatorPrivate::nextWaitingFor> waitingFor;

    QRecursionNode recursion;
    QVariantMap initialProperties;

    void clear();

    void forceCompletion(QQmlInstantiationInterrupt &i);
    void incubate(QQmlInstantiationInterrupt &i);
    void incubateCppBasedComponent(QQmlComponent *component, QQmlContext *context);
    RequiredProperties *requiredProperties();
    bool hadTopLevelRequiredProperties() const;
};

QT_END_NAMESPACE

#endif // QQMLINCUBATOR_P_H

