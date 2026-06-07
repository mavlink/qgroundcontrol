// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCOMPONENTANDALIASRESOLVER_P_H
#define QQMLCOMPONENTANDALIASRESOLVER_P_H

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

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlerror.h>

#include <QtCore/qglobal.h>
#include <QtCore/qhash.h>

#include <private/qqmltypeloader_p.h>
#include <private/qqmlpropertycachecreator_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQmlTypeCompiler);

// This class primarily resolves component boundaries in a document.
// With the information about boundaries, it then goes on to resolve aliases and generalized
// group properties. Both rely on IDs as first part of their expressions and the IDs have
// to be located in surrounding components. That's why we have to do this with the component
// boundaries in mind.

class QQmlComponentAndAliasResolverBase
{
    Q_DECLARE_TR_FUNCTIONS(QQmlComponentAndAliasResolverBase)
};

template<typename ObjectContainer>
class QQmlComponentAndAliasResolver : public QQmlComponentAndAliasResolverBase
{
public:
    using CompiledObject = typename ObjectContainer::CompiledObject;
    using CompiledBinding = typename ObjectContainer::CompiledBinding;

    QQmlComponentAndAliasResolver(
            ObjectContainer *compiler,
            QQmlPropertyCacheVector *propertyCaches);

    [[nodiscard]] QQmlError resolve(int root = 0);

private:
    enum AliasResolutionResult {
        NoAliasResolved,
        SomeAliasesResolved,
        AllAliasesResolved
    };

    // To be specialized for each container
    void allocateNamedObjects(CompiledObject *object) const;
    void setObjectId(int index) const;
    [[nodiscard]] bool markAsComponent(int index) const;
    [[nodiscard]] AliasResolutionResult resolveAliasesInObject(
            const CompiledObject &component, int objectIndex,
            QQmlPropertyCacheAliasCreator<ObjectContainer> *aliasCacheCreator, QQmlError *error);
    [[nodiscard]] bool appendAliasToPropertyCache(
            const CompiledObject *component, const QV4::CompiledData::Alias *alias, int objectIndex,
            int aliasIndex, int encodedPropertyIndex,
            QQmlPropertyCacheAliasCreator<ObjectContainer> *aliasCacheCreator, QQmlError *error)
    {
        *error = aliasCacheCreator->appendAliasToPropertyCache(
                *component, *alias, objectIndex, aliasIndex, encodedPropertyIndex);
        resolvedAliases.insert(alias);
        return !error->isValid();
    }


    void resolveGeneralizedGroupProperty(const CompiledObject &component, CompiledBinding *binding);
    [[nodiscard]] bool wrapImplicitComponent(CompiledBinding *binding);

    [[nodiscard]] QQmlError findAndRegisterImplicitComponents(
            const CompiledObject *obj, const QQmlPropertyCache::ConstPtr &propertyCache);
    [[nodiscard]] QQmlError collectIdsAndAliases(int objectIndex);
    [[nodiscard]] QQmlError resolveAliases(int componentIndex);
    void resolveGeneralizedGroupProperties(int componentIndex);
    [[nodiscard]] QQmlError resolveComponentsInInlineComponentRoot(int root);

    QString stringAt(int idx) const { return m_compiler->stringAt(idx); }
    QV4::ResolvedTypeReference *resolvedType(int id) const { return m_compiler->resolvedType(id); }

    [[nodiscard]] QQmlError error(
            const QV4::CompiledData::Location &location,
            const QString &description)
    {
        QQmlError error;
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
        error.setDescription(description);
        error.setUrl(m_compiler->url());
        return error;
    }

    template<typename Token>
    [[nodiscard]] QQmlError error(Token token, const QString &description)
    {
        return error(token->location, description);
    }

    static bool isUsableComponent(const QMetaObject *metaObject)
    {
        // The metaObject is a component we're interested in if it either is a QQmlComponent itself
        // or if any of its parents is a QQmlAbstractDelegateComponent. We don't want to include
        // qqmldelegatecomponent_p.h because it belongs to QtQmlModels.

        if (metaObject == &QQmlComponent::staticMetaObject)
            return true;

        for (; metaObject; metaObject = metaObject->superClass()) {
            if (qstrcmp(metaObject->className(), "QQmlAbstractDelegateComponent") == 0)
                return true;
        }

        return false;
    }

    ObjectContainer *m_compiler = nullptr;

    // Implicit component insertion may have added objects and thus we also need
    // to extend the symmetric propertyCaches. Therefore, non-const propertyCaches.
    QQmlPropertyCacheVector *m_propertyCaches = nullptr;

    // indices of the objects that are actually Component {}
    QVector<quint32> m_componentRoots;
    QVector<int> m_objectsWithAliases;
    QVector<CompiledBinding *> m_generalizedGroupProperties;
    QSet<const QV4::CompiledData::Alias *> resolvedAliases;
    typename ObjectContainer::IdToObjectMap m_idToObjectIndex;
};

template<typename ObjectContainer>
QQmlComponentAndAliasResolver<ObjectContainer>::QQmlComponentAndAliasResolver(
        ObjectContainer *compiler,
        QQmlPropertyCacheVector *propertyCaches)
    : m_compiler(compiler)
    , m_propertyCaches(propertyCaches)
{
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::findAndRegisterImplicitComponents(
        const CompiledObject *obj, const QQmlPropertyCache::ConstPtr &propertyCache)
{
    QQmlPropertyResolver propertyResolver(propertyCache);

    const QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1
            ? propertyCache->parent()->defaultProperty()
            : propertyCache->defaultProperty();

    for (auto binding = obj->bindingsBegin(), end = obj->bindingsEnd(); binding != end; ++binding) {
        if (binding->type() != QV4::CompiledData::Binding::Type_Object)
            continue;
        if (binding->hasFlag(QV4::CompiledData::Binding::IsSignalHandlerObject))
            continue;

        auto targetObject = m_compiler->objectAt(binding->value.objectIndex);
        auto typeReference = resolvedType(targetObject->inheritedTypeNameIndex);
        Q_ASSERT(typeReference);

        const QMetaObject *firstMetaObject = nullptr;
        const auto type = typeReference->type();
        if (type.isValid())
            firstMetaObject = type.metaObject();
        else if (const auto compilationUnit = typeReference->compilationUnit())
            firstMetaObject = compilationUnit->rootPropertyCache()->firstCppMetaObject();
        if (isUsableComponent(firstMetaObject))
            continue;

        // if here, not a QQmlComponent, so needs wrapping
        const QQmlPropertyData *pd = nullptr;
        if (binding->propertyNameIndex != quint32(0)) {
            bool notInRevision = false;
            pd = propertyResolver.property(stringAt(binding->propertyNameIndex), &notInRevision);
        } else {
            pd = defaultProperty;
        }
        if (!pd || !pd->isQObject())
            continue;

        // If the version is given, use it and look up by QQmlType.
        // Otherwise, make sure we look up by metaobject.
        // TODO: Is this correct?
        QQmlPropertyCache::ConstPtr pc = pd->typeVersion().hasMinorVersion()
                ? QQmlMetaType::rawPropertyCacheForType(pd->propType(), pd->typeVersion())
                : QQmlMetaType::rawPropertyCacheForType(pd->propType());
        const QMetaObject *mo = pc ? pc->firstCppMetaObject() : nullptr;
        while (mo) {
            if (mo == &QQmlComponent::staticMetaObject)
                break;
            mo = mo->superClass();
        }

        if (!mo)
            continue;

        if (!wrapImplicitComponent(binding))
            return error(binding, QQmlComponentAndAliasResolverBase::tr("Cannot wrap implicit component"));
    }

    return QQmlError();
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::resolveComponentsInInlineComponentRoot(
        int root)
{
    // Find implicit components in the inline component itself. Also warn about inline
    // components being explicit components.

    const auto rootObj = m_compiler->objectAt(root);
    Q_ASSERT(rootObj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot));

    if (const int typeName = rootObj->inheritedTypeNameIndex) {
        const auto *tref = resolvedType(typeName);
        Q_ASSERT(tref);
        if (tref->type().metaObject() == &QQmlComponent::staticMetaObject) {
            qCWarning(lcQmlTypeCompiler).nospace().noquote()
                    << m_compiler->url().toString() << ":" << rootObj->location.line() << ":"
                    << rootObj->location.column()
                    << ": Using a Component as the root of an inline component is deprecated: "
                       "inline components are "
                       "automatically wrapped into Components when needed.";
            return QQmlError();
        }
    }

    const QQmlPropertyCache::ConstPtr rootCache = m_propertyCaches->at(root);
    Q_ASSERT(rootCache);

    return findAndRegisterImplicitComponents(rootObj, rootCache);
}

// Resolve ignores everything relating to inline components, except for implicit components.
template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::resolve(int root)
{
    // Detect real Component {} objects as well as implicitly defined components, such as
    //     someItemDelegate: Item {}
    // In the implicit case Item is surrounded by a synthetic Component {} because the property
    // on the left hand side is of QQmlComponent type.
    const int objCountWithoutSynthesizedComponents = m_compiler->objectCount();

    if (root != 0) {
        const QQmlError error = resolveComponentsInInlineComponentRoot(root);
        if (error.isValid())
            return error;
    }

    // root+1, as ic root is handled at the end
    const int startObjectIndex = root == 0 ? root : root+1;

    for (int i = startObjectIndex; i < objCountWithoutSynthesizedComponents; ++i) {
        auto obj = m_compiler->objectAt(i);
        const bool isInlineComponentRoot
                = obj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot);
        const bool isPartOfInlineComponent
                = obj->hasFlag(QV4::CompiledData::Object::IsPartOfInlineComponent);
        QQmlPropertyCache::ConstPtr cache = m_propertyCaches->at(i);

        if (root == 0) {
            // normal component root, skip over anything inline component related
            if (isInlineComponentRoot || isPartOfInlineComponent)
                continue;
        } else if (!isPartOfInlineComponent || isInlineComponentRoot) {
            // When handling an inline component, stop where the inline component ends
            // Note: We do not support nested inline components. Therefore, isInlineComponentRoot
            //       tells us that the element after the current inline component is again an
            //       inline component
            break;
        }

        if (obj->inheritedTypeNameIndex == 0 && !cache)
            continue;

        bool isExplicitComponent = false;
        if (obj->inheritedTypeNameIndex) {
            auto *tref = resolvedType(obj->inheritedTypeNameIndex);
            Q_ASSERT(tref);
            if (tref->type().metaObject() == &QQmlComponent::staticMetaObject)
                isExplicitComponent = true;
        }

        if (!isExplicitComponent) {
            if (cache) {
                const QQmlError error = findAndRegisterImplicitComponents(obj, cache);
                if (error.isValid())
                    return error;
            }
            continue;
        }

        if (!markAsComponent(i))
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Cannot mark object as component"));

        // check if this object is the root
        if (i == 0) {
            if (isExplicitComponent)
                qCWarning(lcQmlTypeCompiler).nospace().noquote()
                        << m_compiler->url().toString() << ":" << obj->location.line() << ":"
                        << obj->location.column()
                        << ": Using a Component as the root of a QML document is deprecated: types "
                           "defined in qml documents are "
                           "automatically wrapped into Components when needed.";
        }

        if (obj->functionCount() > 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Component objects cannot declare new functions."));
        if (obj->propertyCount() > 0 || obj->aliasCount() > 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Component objects cannot declare new properties."));
        if (obj->signalCount() > 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Component objects cannot declare new signals."));

        if (obj->bindingCount() == 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Cannot create empty component specification"));

        const auto rootBinding = obj->bindingsBegin();
        const auto bindingsEnd = obj->bindingsEnd();

        // Produce the more specific "no properties" error rather than the "invalid body" error
        // where possible.
        for (auto b = rootBinding; b != bindingsEnd; ++b) {
            if (b->propertyNameIndex == 0)
                continue;

            return error(b, QQmlComponentAndAliasResolverBase::tr("Component elements may not contain properties other than id"));
        }

        if (auto b = rootBinding;
                b->type() != QV4::CompiledData::Binding::Type_Object || ++b != bindingsEnd) {
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Invalid component body specification"));
        }

        // For the root object, we are going to collect ids/aliases and resolve them for as a
        // separate last pass.
        if (i != 0)
            m_componentRoots.append(i);
    }

    for (int i = 0; i < m_componentRoots.size(); ++i) {
        CompiledObject *component = m_compiler->objectAt(m_componentRoots.at(i));
        const auto rootBinding = component->bindingsBegin();

        m_idToObjectIndex.clear();
        m_objectsWithAliases.clear();
        m_generalizedGroupProperties.clear();

        if (const QQmlError error = collectIdsAndAliases(rootBinding->value.objectIndex);
                error.isValid()) {
            return error;
        }

        allocateNamedObjects(component);

        if (const QQmlError error = resolveAliases(m_componentRoots.at(i)); error.isValid())
            return error;

        resolveGeneralizedGroupProperties(m_componentRoots.at(i));
    }

    // Collect ids and aliases for root
    m_idToObjectIndex.clear();
    m_objectsWithAliases.clear();
    m_generalizedGroupProperties.clear();

    if (const QQmlError error = collectIdsAndAliases(root); error.isValid())
        return error;

    allocateNamedObjects(m_compiler->objectAt(root));
    if (const QQmlError error = resolveAliases(root); error.isValid())
        return error;

    resolveGeneralizedGroupProperties(root);
    return QQmlError();
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::collectIdsAndAliases(int objectIndex)
{
    auto obj = m_compiler->objectAt(objectIndex);

    if (obj->idNameIndex != 0) {
        if (m_idToObjectIndex.contains(obj->idNameIndex))
            return error(obj->locationOfIdProperty, QQmlComponentAndAliasResolverBase::tr("id is not unique"));
        setObjectId(objectIndex);
        m_idToObjectIndex.insert(obj->idNameIndex, objectIndex);
    }

    if (obj->aliasCount() > 0)
        m_objectsWithAliases.append(objectIndex);

    // Stop at Component boundary
    if (obj->hasFlag(QV4::CompiledData::Object::IsComponent) && objectIndex != /*root object*/0)
        return QQmlError();

    for (auto binding = obj->bindingsBegin(), end = obj->bindingsEnd();
         binding != end; ++binding) {
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_GroupProperty: {
            const auto *inner = m_compiler->objectAt(binding->value.objectIndex);
            if (m_compiler->stringAt(inner->inheritedTypeNameIndex).isEmpty()) {
                const auto cache = m_propertyCaches->at(objectIndex);
                if (!cache || !cache->property(
                            m_compiler->stringAt(binding->propertyNameIndex), nullptr, nullptr)) {
                    m_generalizedGroupProperties.append(binding);
                }
            }
        }
        Q_FALLTHROUGH();
        case QV4::CompiledData::Binding::Type_Object:
        case QV4::CompiledData::Binding::Type_AttachedProperty:
            if (const QQmlError error = collectIdsAndAliases(binding->value.objectIndex);
                    error.isValid()) {
                return error;
            }
            break;
        default:
            break;
        }
    }

    return QQmlError();
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::resolveAliases(int componentIndex)
{
    if (m_objectsWithAliases.isEmpty())
        return QQmlError();

    QQmlPropertyCacheAliasCreator<ObjectContainer> aliasCacheCreator(m_propertyCaches, m_compiler);

    bool atLeastOneAliasResolved;
    do {
        atLeastOneAliasResolved = false;
        QVector<int> pendingObjects;

        for (int objectIndex: std::as_const(m_objectsWithAliases)) {

            QQmlError error;
            const auto &component = *m_compiler->objectAt(componentIndex);
            const auto result
                    = resolveAliasesInObject(component, objectIndex, &aliasCacheCreator, &error);
            if (error.isValid())
                return error;

            if (result == AllAliasesResolved) {
                atLeastOneAliasResolved = true;
            } else if (result == SomeAliasesResolved) {
                atLeastOneAliasResolved = true;
                pendingObjects.append(objectIndex);
            } else {
                pendingObjects.append(objectIndex);
            }
        }
        qSwap(m_objectsWithAliases, pendingObjects);
    } while (!m_objectsWithAliases.isEmpty() && atLeastOneAliasResolved);

    if (!atLeastOneAliasResolved && !m_objectsWithAliases.isEmpty()) {
        const CompiledObject *obj = m_compiler->objectAt(m_objectsWithAliases.first());
        for (auto alias = obj->aliasesBegin(), end = obj->aliasesEnd(); alias != end; ++alias) {
            if (!resolvedAliases.contains(alias))
                return error(alias->location, QQmlComponentAndAliasResolverBase::tr("Circular alias reference detected"));
        }
    }

    return QQmlError();
}

template<typename ObjectContainer>
void QQmlComponentAndAliasResolver<ObjectContainer>::resolveGeneralizedGroupProperties(
        int componentIndex)
{
    const auto &component = *m_compiler->objectAt(componentIndex);
    for (CompiledBinding *binding : m_generalizedGroupProperties)
        resolveGeneralizedGroupProperty(component, binding);
}

QT_END_NAMESPACE

#endif // QQMLCOMPONENTANDALIASRESOLVER_P_H
