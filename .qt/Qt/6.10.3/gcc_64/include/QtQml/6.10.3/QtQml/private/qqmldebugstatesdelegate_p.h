// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGSTATESDELEGATE_P_H
#define QQMLDEBUGSTATESDELEGATE_P_H

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

#include <QtQml/qtqmlglobal.h>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

#if !QT_CONFIG(qml_debug)

class QQmlDebugStatesDelegate {};

#else

class QQmlContext;
class QQmlProperty;
class QObject;
class QString;
class QVariant;

class QQmlDebugStatesDelegate
{
protected:
    QQmlDebugStatesDelegate() {}

public:
    virtual ~QQmlDebugStatesDelegate() {}

    virtual void buildStatesList(bool cleanList,
                                 const QList<QPointer<QObject> > &instances) = 0;
    virtual void updateBinding(QQmlContext *context,
                               const QQmlProperty &property,
                               const QVariant &expression, bool isLiteralValue,
                               const QString &fileName, int line, int column,
                               bool *inBaseState) = 0;
    virtual bool setBindingForInvalidProperty(QObject *object,
                                              const QString &propertyName,
                                              const QVariant &expression,
                                              bool isLiteralValue) = 0;
    virtual void resetBindingForInvalidProperty(QObject *object,
                                                const QString &propertyName) = 0;

private:
    Q_DISABLE_COPY(QQmlDebugStatesDelegate)
};

#endif

QT_END_NAMESPACE

#endif // QQMLDEBUGSTATESDELEGATE_P_H
