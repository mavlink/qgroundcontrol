// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLEXPRESSION_P_H
#define QQMLEXPRESSION_P_H

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

#include "qqmlexpression.h"

#include <private/qqmlengine_p.h>
#include <private/qfieldlist_p.h>
#include <private/qqmljavascriptexpression_p.h>

QT_BEGIN_NAMESPACE

class QQmlExpression;
class QString;
class QQmlExpressionPrivate : public QObjectPrivate,
                              public QQmlJavaScriptExpression
{
    Q_DECLARE_PUBLIC(QQmlExpression)
public:
    QQmlExpressionPrivate();
    ~QQmlExpressionPrivate() override;

    void init(const QQmlRefPointer<QQmlContextData> &, const QString &, QObject *);
    void init(const QQmlRefPointer<QQmlContextData> &, QV4::Function *runtimeFunction, QObject *);

    QVariant value(bool *isUndefined = nullptr);

    QV4::ReturnedValue v4value(bool *isUndefined = nullptr);
    bool mustCaptureBindableProperty() const final {return true;}

    static inline QQmlExpressionPrivate *get(QQmlExpression *expr);
    static inline QQmlExpression *get(QQmlExpressionPrivate *expr);

    void _q_notify();

    bool expressionFunctionValid:1;

    // Inherited from QQmlJavaScriptExpression
    QString expressionIdentifier() const override;
    void expressionChanged() override;

    QString expression;

    QString url; // This is a QString for a reason.  QUrls are slooooooow...
    quint16 line;
    quint16 column;
    QString name; //function name, hint for the debugger
};

QQmlExpressionPrivate *QQmlExpressionPrivate::get(QQmlExpression *expr)
{
    return static_cast<QQmlExpressionPrivate *>(QObjectPrivate::get(expr));
}

QQmlExpression *QQmlExpressionPrivate::get(QQmlExpressionPrivate *expr)
{
    return expr->q_func();
}


QT_END_NAMESPACE

#endif // QQMLEXPRESSION_P_H
