// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCOMPONENT_P_H
#define QQMLCOMPONENT_P_H

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

#include "qqmlcomponent.h"

#include "qqmlengine_p.h"
#include "qqmlerror.h"
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmlguardedcontextdata_p.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/qtclasshelpermacros.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQmlEngine;

class QQmlComponentAttached;
class Q_QML_EXPORT QQmlComponentPrivate
    : public QObjectPrivate, public QQmlTypeData::TypeDataCallback
{
    Q_DECLARE_PUBLIC(QQmlComponent)

public:
    enum CreateBehavior
    {
        CreateDefault,
        CreateWarnAboutRequiredProperties,
    };

    struct AnnotatedQmlError
    {
        AnnotatedQmlError() = default;
        AnnotatedQmlError(QQmlError error) : error(std::move(error)) {}
        AnnotatedQmlError(QQmlError error, bool transient)
            : error(std::move(error)), isTransient(transient)
        {
        }

        QQmlError error;
        bool isTransient = false; // tells if the error is temporary (e.g. unset required property)
    };

    struct ConstructionState
    {
    public:
        ConstructionState() = default;
        inline ConstructionState(ConstructionState &&other) noexcept;
        inline ~ConstructionState();

        QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QQmlComponentPrivate::ConstructionState)
        void swap(ConstructionState &other)
        {
            m_creatorOrRequiredProperties.swap(other.m_creatorOrRequiredProperties);
        }

        inline void ensureRequiredPropertyStorage(QObject *target);
        inline RequiredProperties *requiredProperties() const;
        inline void addPendingRequiredProperty(
                const QObject *object, const QQmlPropertyData *propData,
                const RequiredPropertyInfo &info);
        inline bool hasUnsetRequiredProperties() const;
        inline void clearRequiredProperties();

        inline void appendErrors(const QList<QQmlError> &qmlErrors);
        inline void appendCreatorErrors();

        inline QQmlObjectCreator *creator();
        inline const QQmlObjectCreator *creator() const;
        inline void clear();
        inline bool hasCreator() const;
        inline QQmlObjectCreator *initCreator(
                const QQmlRefPointer<QQmlContextData> &parentContext,
                const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                const QQmlRefPointer<QQmlContextData> &creationContext,
                const QString &inlineComponentName);

        QList<AnnotatedQmlError> errors;
        inline bool isCompletePending() const;
        inline void setCompletePending(bool isPending);

        QObject *target() const
        {
            if (m_creatorOrRequiredProperties.isNull())
                return nullptr;

            if (m_creatorOrRequiredProperties.isT1()) {
                const auto &objects = m_creatorOrRequiredProperties.asT1()->allCreatedObjects();
                return objects.empty() ? nullptr : objects.at(0);
            }

            Q_ASSERT(m_creatorOrRequiredProperties.isT2());
            return m_creatorOrRequiredProperties.asT2()->target;
        }

    private:
        Q_DISABLE_COPY(ConstructionState)
        QBiPointer<QQmlObjectCreator, RequiredPropertiesAndTarget> m_creatorOrRequiredProperties;
    };

    using DeferredState = std::vector<ConstructionState>;

    void loadUrl(const QUrl &newUrl,
                 QQmlComponent::CompilationMode mode = QQmlComponent::PreferSynchronous);

    QQmlType loadedType() const { return m_loadHelper ? m_loadHelper->type() : QQmlType(); }

    QObject *beginCreate(QQmlRefPointer<QQmlContextData>);
    void completeCreate();
    void initializeObjectWithInitialProperties(
            QV4::QmlContext *qmlContext, const QV4::Value &valuemap, QObject *toCreate,
            RequiredProperties *requiredProperties);
    static void setInitialProperties(
            QV4::ExecutionEngine *engine, QV4::QmlContext *qmlContext, const QV4::Value &o,
            const QV4::Value &v, RequiredProperties *requiredProperties, QObject *createdComponent,
            const QQmlObjectCreator *creator);
    static QQmlError unsetRequiredPropertyToQQmlError(
            const RequiredPropertyInfo &unsetRequiredProperty);

    virtual void incubateObject(
            QQmlIncubator *incubationTask,
            QQmlComponent *component,
            QQmlEngine *engine,
            const QQmlRefPointer<QQmlContextData> &context,
            const QQmlRefPointer<QQmlContextData> &forContext);

    void typeDataReady(QQmlTypeData *) override;
    void typeDataProgress(QQmlTypeData *, qreal) override;

    void fromTypeData(const QQmlRefPointer<QQmlTypeData> &data);

    bool hadTopLevelRequiredProperties() const;

    static void beginDeferred(
            QQmlEnginePrivate *enginePriv, QObject *object, DeferredState* deferredState);
    static void completeDeferred(
            QQmlEnginePrivate *enginePriv, DeferredState *deferredState);

    static void complete(QQmlEnginePrivate *enginePriv, ConstructionState *state);
    static QQmlProperty removePropertyFromRequired(
            QObject *createdComponent, const QString &name,
            RequiredProperties *requiredProperties, QQmlEngine *engine,
            bool *wasInRequiredProperties = nullptr);

    void clear();

    static QQmlComponentPrivate *get(QQmlComponent *c) {
        return static_cast<QQmlComponentPrivate *>(QObjectPrivate::get(c));
    }

    QObject *doBeginCreate(QQmlComponent *q, QQmlContext *context);
    bool setInitialProperty(QObject *component, const QString &name, const QVariant& value);

    QObject *createWithProperties(QObject *parent, const QVariantMap &properties,
                                  QQmlContext *context, CreateBehavior behavior = CreateDefault,
                                  bool createFromQml = false);

    bool isBound() const { return m_compilationUnit && (m_compilationUnit->componentsAreBound()); }
    void prepareLoadFromModule(
            QAnyStringView uri, QAnyStringView typeName, QQmlTypeLoader::Mode mode);
    void completeLoadFromModule(
            QAnyStringView uri, QAnyStringView typeName);

    void setProgress(qreal progress)
    {
        if (progress != m_progress) {
            m_progress = progress;
            emit q_func()->progressChanged(progress);
        }
    }
    void setCreationContext(QQmlRefPointer<QQmlContextData> creationContext)
    {
        m_creationContext = std::move(creationContext);
    }

    QQmlType loadHelperType() const { return m_loadHelper->type(); }
    bool hasUnsetRequiredProperties() const { return m_state.hasUnsetRequiredProperties(); }
    RequiredProperties *requiredProperties() const { return m_state.requiredProperties(); }
    const QQmlObjectCreator *creator() const { return m_state.creator(); }

    QQmlEngine *engine() const { return m_engine; }
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit() const
    {
        return m_compilationUnit;
    }

private:
    ConstructionState m_state;
    QQmlGuardedContextData m_creationContext;

    QQmlRefPointer<QV4::ExecutableCompilationUnit> m_compilationUnit;
    QQmlRefPointer<QQmlTypeData> m_typeData;
    QQmlRefPointer<LoadHelper> m_loadHelper;
    std::unique_ptr<QString> m_inlineComponentName;
    QQmlEngine *m_engine = nullptr;

    QUrl m_url;
    qreal m_progress = 0;

    /* points to the sub-object in a QML file that should be instantiated
       used create instances of QtQml's Component type and indirectly for inline components */
    int m_start = -1;
};

QQmlComponentPrivate::ConstructionState::~ConstructionState()
{
    if (m_creatorOrRequiredProperties.isT1())
        delete m_creatorOrRequiredProperties.asT1();
    else
        delete m_creatorOrRequiredProperties.asT2();
}

QQmlComponentPrivate::ConstructionState::ConstructionState(ConstructionState &&other) noexcept
{
    errors = std::move(other.errors);
    m_creatorOrRequiredProperties = std::exchange(other.m_creatorOrRequiredProperties, {});
}

/*!
   \internal A list of pending required properties that need
   to be set in order for object construction to be successful.
 */
inline RequiredProperties *QQmlComponentPrivate::ConstructionState::requiredProperties() const
{
    if (m_creatorOrRequiredProperties.isNull())
        return nullptr;
    else if (m_creatorOrRequiredProperties.isT1())
        return m_creatorOrRequiredProperties.asT1()->requiredProperties();
    else
        return m_creatorOrRequiredProperties.asT2();
}

inline void QQmlComponentPrivate::ConstructionState::addPendingRequiredProperty(
        const QObject *object, const QQmlPropertyData *propData, const RequiredPropertyInfo &info)
{
    Q_ASSERT(requiredProperties());
    requiredProperties()->insert({object, propData}, info);
}

inline bool QQmlComponentPrivate::ConstructionState::hasUnsetRequiredProperties() const {
    auto properties = const_cast<ConstructionState *>(this)->requiredProperties();
    return properties && !properties->isEmpty();
}

inline void QQmlComponentPrivate::ConstructionState::clearRequiredProperties()
{
    if (auto reqProps = requiredProperties())
        reqProps->clear();
}

inline void QQmlComponentPrivate::ConstructionState::appendErrors(const QList<QQmlError> &qmlErrors)
{
    for (const QQmlError &e : qmlErrors)
        errors.emplaceBack(e);
}

//! \internal Moves errors from creator into construction state itself
inline void QQmlComponentPrivate::ConstructionState::appendCreatorErrors()
{
    if (!hasCreator())
        return;
    auto creatorErrorCount = creator()->errors.size();
    if (creatorErrorCount == 0)
        return;
    auto existingErrorCount = errors.size();
    errors.resize(existingErrorCount + creatorErrorCount);
    for (qsizetype i = 0; i < creatorErrorCount; ++i)
        errors[existingErrorCount + i] = AnnotatedQmlError { std::move(creator()->errors[i]) };
    creator()->errors.clear();
}

inline QQmlObjectCreator *QQmlComponentPrivate::ConstructionState::creator()
{
    if (m_creatorOrRequiredProperties.isT1())
        return m_creatorOrRequiredProperties.asT1();
    return nullptr;
}

inline const QQmlObjectCreator *QQmlComponentPrivate::ConstructionState::creator() const
{
    if (m_creatorOrRequiredProperties.isT1())
        return m_creatorOrRequiredProperties.asT1();
    return nullptr;
}

inline bool QQmlComponentPrivate::ConstructionState::hasCreator() const
{
    return creator() != nullptr;
}

inline void QQmlComponentPrivate::ConstructionState::clear()
{
    if (m_creatorOrRequiredProperties.isT1()) {
        delete m_creatorOrRequiredProperties.asT1();
        m_creatorOrRequiredProperties = static_cast<QQmlObjectCreator *>(nullptr);
    }
}

inline QQmlObjectCreator *QQmlComponentPrivate::ConstructionState::initCreator(
        const QQmlRefPointer<QQmlContextData> &parentContext,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QQmlRefPointer<QQmlContextData> &creationContext,
        const QString &inlineComponentName)
{
    if (m_creatorOrRequiredProperties.isT1())
        delete m_creatorOrRequiredProperties.asT1();
    else
        delete m_creatorOrRequiredProperties.asT2();
    m_creatorOrRequiredProperties = new QQmlObjectCreator(
            parentContext, compilationUnit, creationContext, inlineComponentName);
    return m_creatorOrRequiredProperties.asT1();
}

inline bool QQmlComponentPrivate::ConstructionState::isCompletePending() const
{
    return m_creatorOrRequiredProperties.flag();
}

inline void QQmlComponentPrivate::ConstructionState::setCompletePending(bool isPending)
{
    m_creatorOrRequiredProperties.setFlagValue(isPending);
}

/*!
    \internal
    This is meant to be used in the context of QQmlComponent::loadFromModule,
    when dealing with a C++ type. In that case, we do not have a creator,
    and need a separate storage for required properties and the target object.
 */
inline void QQmlComponentPrivate::ConstructionState::ensureRequiredPropertyStorage(QObject *target)
{
    Q_ASSERT(m_creatorOrRequiredProperties.isT2() || m_creatorOrRequiredProperties.isNull());
    if (m_creatorOrRequiredProperties.isNull())
        m_creatorOrRequiredProperties = new RequiredPropertiesAndTarget(target);
    else
        m_creatorOrRequiredProperties.asT2()->target = target;
}

QT_END_NAMESPACE

#endif // QQMLCOMPONENT_P_H
