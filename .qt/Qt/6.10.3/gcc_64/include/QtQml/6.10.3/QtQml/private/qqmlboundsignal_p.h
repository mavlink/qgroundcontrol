// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLBOUNDSIGNAL_P_H
#define QQMLBOUNDSIGNAL_P_H

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

#include <QtCore/qmetaobject.h>

#include <private/qqmljavascriptexpression_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlBoundSignalExpression final
    : public QQmlJavaScriptExpression,
      public QQmlRefCounted<QQmlBoundSignalExpression>
{
    friend class QQmlRefCounted<QQmlBoundSignalExpression>;
public:
    QQmlBoundSignalExpression(
            const QObject *target, int index, const QQmlRefPointer<QQmlContextData> &ctxt, QObject *scope,
            const QString &expression, const QString &fileName, quint16 line, quint16 column,
            const QString &handlerName = QString(), const QString &parameterString = QString());

    QQmlBoundSignalExpression(
            const QObject *target, int index, const QQmlRefPointer<QQmlContextData> &ctxt,
            QObject *scopeObject, QV4::Function *function, QV4::ExecutionContext *scope = nullptr);

    // inherited from QQmlJavaScriptExpression.
    QString expressionIdentifier() const override;
    void expressionChanged() override;

    // evaluation of a bound signal expression doesn't return any value
    void evaluate(void **a);

    bool mustCaptureBindableProperty() const final {return true;}

    QString expression() const;
    const QObject *target() const { return m_target; }

private:
    ~QQmlBoundSignalExpression() override;

    void init(const QQmlRefPointer<QQmlContextData> &ctxt, QObject *scope);

    bool expressionFunctionValid() const { return function() != nullptr; }

    int m_index;
    const QObject *m_target;
};

class Q_QML_EXPORT QQmlBoundSignal : public QQmlNotifierEndpoint
{
public:
    QQmlBoundSignal(QObject *target, int signal, QObject *owner, QQmlEngine *engine);
    ~QQmlBoundSignal();

    void removeFromObject();

    QQmlBoundSignalExpression *expression() const;
    void takeExpression(QQmlBoundSignalExpression *);

    void setEnabled(bool enabled);

private:
    friend void QQmlBoundSignal_callback(QQmlNotifierEndpoint *, void **);
    friend class QQmlPropertyPrivate;
    friend class QQmlData;
    friend class QQmlEngineDebugService;

    void addToObject(QObject *owner);

    QQmlBoundSignal **m_prevSignal;
    QQmlBoundSignal  *m_nextSignal;

    bool m_enabled;

    QQmlRefPointer<QQmlBoundSignalExpression> m_expression;
};

class QQmlPropertyObserver : public QPropertyObserver
{
public:
    QQmlPropertyObserver(QQmlBoundSignalExpression *expr);

private:
    QQmlRefPointer<QQmlBoundSignalExpression> expression;
};

QT_END_NAMESPACE

#endif // QQMLBOUNDSIGNAL_P_H
