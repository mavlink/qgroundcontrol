// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLJAVASCRIPTEXPRESSION_P_H
#define QQMLJAVASCRIPTEXPRESSION_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qtaggedpointer.h>
#include <QtQml/qqmlerror.h>
#include <private/qqmlengine_p.h>
#include <QtQml/private/qbipointer_p.h>

QT_BEGIN_NAMESPACE

struct QQmlSourceLocation;

class QQmlDelayedError
{
public:
    inline QQmlDelayedError() : nextError(nullptr), prevError(nullptr) {}
    inline ~QQmlDelayedError() { (void)removeError(); }

    bool addError(QQmlEnginePrivate *);

    Q_REQUIRED_RESULT inline QQmlError removeError() {
        if (prevError) {
            if (nextError) nextError->prevError = prevError;
            *prevError = nextError;
            nextError = nullptr;
            prevError = nullptr;
        }
        return m_error;
    }

    inline bool isValid() const { return m_error.isValid(); }
    inline const QQmlError &error() const { return m_error; }
    inline void clearError() { m_error = QQmlError(); }

    void setErrorLocation(const QQmlSourceLocation &sourceLocation);
    void setErrorDescription(const QString &description);
    void setErrorObject(QObject *object);

    // Call only from catch(...) -- will re-throw if no JS exception
    void catchJavaScriptException(QV4::ExecutionEngine *engine);

private:

    mutable QQmlError m_error;

    QQmlDelayedError  *nextError;
    QQmlDelayedError **prevError;
};

class Q_QML_EXPORT QQmlJavaScriptExpression
{
    Q_DISABLE_COPY_MOVE(QQmlJavaScriptExpression)
public:
    QQmlJavaScriptExpression();
    virtual ~QQmlJavaScriptExpression();

    virtual QString expressionIdentifier() const;
    virtual void expressionChanged() = 0;

    QV4::ReturnedValue evaluate(bool *isUndefined);
    QV4::ReturnedValue evaluate(QV4::CallData *callData, bool *isUndefined);
    bool evaluate(void **a, const QMetaType *types, int argc);

    inline bool notifyOnValueChanged() const;

    void setNotifyOnValueChanged(bool v);
    void resetNotifyOnValueChanged();

    inline QObject *scopeObject() const;
    inline void setScopeObject(QObject *v);

    virtual QQmlSourceLocation sourceLocation() const;

    bool hasContext() const { return m_context != nullptr; }
    bool hasValidContext() const { return m_context && m_context->isValid(); }
    QQmlContext *publicContext() const { return m_context ? m_context->asQQmlContext() : nullptr; }

    QQmlRefPointer<QQmlContextData> context() const { return m_context; }
    void setContext(const QQmlRefPointer<QQmlContextData> &context);

    void insertIntoList(QQmlJavaScriptExpression **listHead)
    {
        m_nextExpression = *listHead;
        if (m_nextExpression)
            m_nextExpression->m_prevExpression = &m_nextExpression;
        m_prevExpression = listHead;
        *listHead = this;
    }

    QV4::Function *function() const { return m_v4Function; }

    virtual void refresh();

    class DeleteWatcher {
    public:
        inline DeleteWatcher(QQmlJavaScriptExpression *);
        inline ~DeleteWatcher();
        inline bool wasDeleted() const;
    private:
        friend class QQmlJavaScriptExpression;
        QObject *_c;
        QQmlJavaScriptExpression **_w;
        QQmlJavaScriptExpression *_s;
    };

    inline bool hasError() const;
    inline bool hasDelayedError() const;
    QQmlError error(QQmlEngine *) const;
    void clearError();
    void clearActiveGuards();
    QQmlDelayedError *delayedError();
    virtual bool mustCaptureBindableProperty() const {return true;}

    static QV4::ReturnedValue evalFunction(
            const QQmlRefPointer<QQmlContextData> &ctxt, QObject *scope, const QString &code,
            const QString &filename, quint16 line);

    QQmlEngine *engine() const { return m_context ? m_context->engine() : nullptr; }
    bool hasUnresolvedNames() const { return m_context && m_context->hasUnresolvedNames(); }

    bool needsPropertyChangeTrigger(QObject *target, int propertyIndex);
    QPropertyChangeTrigger *allocatePropertyChangeTrigger(QObject *target, int propertyIndex);

protected:
    void createQmlBinding(const QQmlRefPointer<QQmlContextData> &ctxt, QObject *scope,
                          const QString &code, const QString &filename, quint16 line);

    void setupFunction(QV4::ExecutionContext *qmlContext, QV4::Function *f);
    void setCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit);

    // We store some flag bits in the following flag pointers.
    //    activeGuards:flag1  - notifyOnValueChanged
    //    activeGuards:flag2  - useSharedContext
    QBiPointer<QObject, DeleteWatcher> m_scopeObject;

    enum GuardTag {
        NoGuardTag,
        NotifyOnValueChanged
    };

    QForwardFieldList<QQmlJavaScriptExpressionGuard, &QQmlJavaScriptExpressionGuard::next, GuardTag> activeGuards;

    enum Tag {
        NoTag,
        InEvaluationLoop
    };

    QTaggedPointer<QQmlDelayedError, Tag> m_error;

private:
    friend class QQmlContextData;
    friend class QQmlPropertyCapture;
    friend void QQmlJavaScriptExpressionGuard_callback(QQmlNotifierEndpoint *, void **);
    friend class QQmlTranslationBindingFromBinding;
    friend class QQmlTranslationBindingFromTranslationInfo;
    friend class QQmlJavaScriptExpressionCapture;

    // Not refcounted as the context will clear the expressions when destructed.
    QQmlContextData *m_context;

    QQmlJavaScriptExpression **m_prevExpression;
    QQmlJavaScriptExpression  *m_nextExpression;

    QV4::PersistentValue m_qmlScope;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> m_compilationUnit;

    QV4::Function *m_v4Function;

protected:
    TriggerList *qpropertyChangeTriggers = nullptr;
};

class Q_QML_EXPORT QQmlPropertyCapture
{
public:
    QQmlPropertyCapture(QQmlEngine *engine, QQmlJavaScriptExpression *e, QQmlJavaScriptExpression::DeleteWatcher *w)
    : engine(engine), expression(e), watcher(w), errorString(nullptr) { }

    ~QQmlPropertyCapture()  {
        Q_ASSERT(guards.isEmpty());
        Q_ASSERT(errorString == nullptr);
    }

    void captureProperty(QQmlNotifier *);
    void captureProperty(QObject *, int, int, bool doNotify = true);
    void captureProperty(QObject *, const QQmlPropertyCache *, const QQmlPropertyData *, bool doNotify = true);
    void captureTranslation();

    QQmlEngine *engine;
    QQmlJavaScriptExpression *expression;
    QQmlJavaScriptExpression::DeleteWatcher *watcher;
    QForwardFieldList<QQmlJavaScriptExpressionGuard, &QQmlJavaScriptExpressionGuard::next> guards;
    QStringList *errorString;

private:
    void captureBindableProperty(QObject *o, const QMetaObject *metaObjectForBindable, int c);
    void captureNonBindableProperty(QObject *o, int n, int c, bool doNotify);
};

QQmlJavaScriptExpression::DeleteWatcher::DeleteWatcher(QQmlJavaScriptExpression *e)
: _c(nullptr), _w(nullptr), _s(e)
{
    if (e->m_scopeObject.isT1()) {
        _w = &_s;
        _c = e->m_scopeObject.asT1();
        e->m_scopeObject = this;
    } else {
        // Another watcher is already registered
        _w = &e->m_scopeObject.asT2()->_s;
    }
}

QQmlJavaScriptExpression::DeleteWatcher::~DeleteWatcher()
{
    Q_ASSERT(*_w == nullptr || (*_w == _s && _s->m_scopeObject.isT2()));
    if (*_w && _s->m_scopeObject.asT2() == this)
        _s->m_scopeObject = _c;
}

bool QQmlJavaScriptExpression::DeleteWatcher::wasDeleted() const
{
    return *_w == nullptr;
}

bool QQmlJavaScriptExpression::notifyOnValueChanged() const
{
    return activeGuards.tag() == NotifyOnValueChanged;
}

QObject *QQmlJavaScriptExpression::scopeObject() const
{
    if (m_scopeObject.isT1()) return m_scopeObject.asT1();
    else return m_scopeObject.asT2()->_c;
}

void QQmlJavaScriptExpression::setScopeObject(QObject *v)
{
    if (m_scopeObject.isT1()) m_scopeObject = v;
    else m_scopeObject.asT2()->_c = v;
}

bool QQmlJavaScriptExpression::hasError() const
{
    return !m_error.isNull() && m_error->isValid();
}

bool QQmlJavaScriptExpression::hasDelayedError() const
{
    return !m_error.isNull();
}

inline void QQmlJavaScriptExpression::clearError()
{
    delete m_error.data();
    m_error = nullptr;
}

QQmlJavaScriptExpressionGuard::QQmlJavaScriptExpressionGuard(QQmlJavaScriptExpression *e)
    : QQmlNotifierEndpoint(QQmlNotifierEndpoint::QQmlJavaScriptExpressionGuard),
      expression(e), next(nullptr)
{
}

QQmlJavaScriptExpressionGuard *
QQmlJavaScriptExpressionGuard::New(QQmlJavaScriptExpression *e,
                                           QQmlEngine *engine)
{
    Q_ASSERT(e);
    return QQmlEnginePrivate::get(engine)->jsExpressionGuardPool.New(e);
}

void QQmlJavaScriptExpressionGuard::Delete()
{
    QRecyclePool<QQmlJavaScriptExpressionGuard>::Delete(this);
}


QT_END_NAMESPACE

#endif // QQMLJAVASCRIPTEXPRESSION_P_H
