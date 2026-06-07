// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QMLTYPESCLASSDESCRIPTION_P_H
#define QMLTYPESCLASSDESCRIPTION_P_H

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

#include <private/qmetatypesjsonprocessor_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qvector.h>
#include <QtCore/qset.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

struct FoundType
{
    enum Origin {
        Unknown,
        OwnTypes,
        ForeignTypes,
    };

    FoundType() = default;
    FoundType(const MetaType &single, Origin origin);

    MetaType native;
    MetaType javaScript;

    Origin nativeOrigin = Unknown;
    Origin javaScriptOrigin = Unknown;

    operator bool() const { return !native.isEmpty() || !javaScript.isEmpty(); }

    MetaType select(const MetaType &category, QAnyStringView relation) const;

};

struct QmlTypesClassDescription
{
    // All the string views in this class are based on string data in the JSON they are parsed from.
    // You must keep the relevant QCborValues alive while the QmlTypesClassDescription exists.

    MetaType resolvedClass;
    QAnyStringView file;
    QAnyStringView className;
    QList<QAnyStringView> primitiveAliases;
    QList<QAnyStringView> elementNames;
    QAnyStringView defaultProp;
    QAnyStringView parentProp;
    QAnyStringView superClass;
    QAnyStringView attachedType;
    QAnyStringView javaScriptExtensionType;
    QAnyStringView nativeExtensionType;
    QAnyStringView sequenceValueType;
    QAnyStringView accessSemantics;
    QList<QTypeRevision> revisions;
    QTypeRevision addedInRevision;
    QTypeRevision removedInRevision;
    bool isCreatable = true;
    bool isStructured = false;
    bool isSingleton = false;
    bool hasCustomParser = false;
    bool isRootClass = false;
    bool extensionIsJavaScript = false;
    bool extensionIsNamespace = false;
    bool enforcesScopedEnums = false;
    QList<QAnyStringView> implementsInterfaces;
    QList<QAnyStringView> deferredNames;
    QList<QAnyStringView> immediateNames;
    int lineNumber = 0;

    enum CollectMode {
        TopLevel,
        SuperClass,
        RelatedType
    };

    void collect(
            const MetaType &classDef, const QVector<MetaType> &types,
            const QVector<MetaType> &foreign, CollectMode mode, QTypeRevision defaultRevision);
    FoundType collectRelated(
            QAnyStringView related, const QVector<MetaType> &types,
            const QVector<MetaType> &foreign, QTypeRevision defaultRevision,
            const QList<QAnyStringView> &namespaces);

    static FoundType findType(
            const QVector<MetaType> &types, const QVector<MetaType> &foreign,
            const QAnyStringView &name, const QList<QAnyStringView> &namespaces);

    void collectLocalAnonymous(
            const MetaType &classDef, const QVector<MetaType> &types,
            const QVector<MetaType> &foreign, QTypeRevision defaultRevision);


private:
    void collectSuperClasses(
            const MetaType &classDef, const QVector<MetaType> &types,
            const QVector<MetaType> &foreign, CollectMode mode, QTypeRevision defaultRevision);
    void collectInterfaces(const MetaType &classDef);

    void handleRegisterEnumClassesUnscoped(const MetaType &classDef, QAnyStringView value);
};

struct ResolvedTypeAlias
{
    ResolvedTypeAlias(QAnyStringView alias, const QList<UsingDeclaration> &usingDeclarations);

    QAnyStringView type;
    bool isList = false;
    bool isPointer = false;
    bool isConstant = false;

private:
    void handleVoid();
    void handleList();
    void handlePointer();
    void handleConst();
};

QT_END_NAMESPACE
#endif // QMLTYPESCLASSDESCRIPTION_P_H
