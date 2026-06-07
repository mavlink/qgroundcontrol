// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef METATYPESJSONPROCESSOR_P_H
#define METATYPESJSONPROCESSOR_P_H

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

#include <private/qduplicatetracker_p.h>

#include <QtCore/qcbormap.h>
#include <QtCore/qstring.h>
#include <QtCore/qtyperevision.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

// With all the QAnyStringViews in this file we rely on the Cbor data to stay
// in place if you don't change the Cbor contents. We assume that Cbor data
// is implicitly shared so that merely copying const Cbor objects does not copy
// the contents.

enum class Access { Public, Protected, Private };

struct BaseType
{
    using Container = QVarLengthArray<BaseType, 1>;

    BaseType() = default;
    BaseType(const QCborMap &cbor);

    QAnyStringView name;
    Access access;
};

struct ClassInfo
{
    using Container = std::vector<ClassInfo>;

    ClassInfo() = default;
    ClassInfo(const QCborMap &cbor);

    QAnyStringView name;
    QAnyStringView value;
};

struct Interface
{
    using Container = QVarLengthArray<Interface, 1>;

    Interface() = default;
    Interface(const QCborValue &cbor);

    QAnyStringView className;
};

struct Property
{
    using Container = std::vector<Property>;

    Property() = default;
    Property(const QCborMap &cbor);

    QAnyStringView name;
    QAnyStringView type;

    QAnyStringView member;
    QAnyStringView read;
    QAnyStringView write;
    QAnyStringView reset;
    QAnyStringView notify;
    QAnyStringView bindable;

    QAnyStringView privateClass;

    int index = -1;
    int lineNumber = 0;

    QTypeRevision revision;

    bool isFinal = false;
    bool isConstant = false;
    bool isRequired = false;
};

struct Argument
{
    using Container = std::vector<Argument>;

    Argument() = default;
    Argument(const QCborMap &cbor);

    QAnyStringView name;
    QAnyStringView type;
};

struct Method
{
    using Container = std::vector<Method>;
    static constexpr int InvalidIndex = std::numeric_limits<int>::min();

    Method() = default;
    Method(const QCborMap &cbor, bool isConstructor);

    QAnyStringView name;

    Argument::Container arguments;
    QAnyStringView returnType;

    int index = InvalidIndex;
    int lineNumber = 0;

    QTypeRevision revision;

    Access access = Access::Public;

    bool isCloned = false;
    bool isJavaScriptFunction = false;
    bool isConstructor = false;
    bool isConst = false;
};

struct Enum
{
    using Container = std::vector<Enum>;

    Enum() = default;
    Enum(const QCborMap &cbor);

    QAnyStringView name;
    QAnyStringView alias;
    QAnyStringView type;

    QList<QAnyStringView> values;

    int lineNumber = 0;
    bool isFlag = false;
    bool isClass = false;
};

struct MetaTypePrivate
{
    Q_DISABLE_COPY_MOVE(MetaTypePrivate)

    enum Kind : quint8 { Object, Gadget, Namespace, Unknown };

    MetaTypePrivate() = default;
    MetaTypePrivate(const QCborMap &cbor, const QString &inputFile);

    const QCborMap cbor; // need to keep this to hold on to the strings
    const QString inputFile;

    QAnyStringView className;
    QAnyStringView qualifiedClassName;
    BaseType::Container superClasses;
    ClassInfo::Container classInfos;
    Interface::Container ifaces;

    Property::Container properties;

    Method::Container methods;
    Method::Container sigs;
    Method::Container constructors;

    Enum::Container enums;

    Kind kind = Unknown;
    int lineNumber = 0;
};

class MetaType
{
public:
    using Kind = MetaTypePrivate::Kind;

    MetaType() = default;
    MetaType(const QCborMap &cbor, const QString &inputFile);

    bool isEmpty() const { return d == &s_empty; }

    QString inputFile() const { return d->inputFile; }
    int lineNumber() const { return d->lineNumber; }
    QAnyStringView className() const { return d->className; }
    QAnyStringView qualifiedClassName() const { return d->qualifiedClassName; }
    const BaseType::Container &superClasses() const { return d->superClasses; }
    const ClassInfo::Container &classInfos() const { return d->classInfos; }
    const Interface::Container &ifaces() const { return d->ifaces; }

    const Property::Container &properties() const { return d->properties; }
    const Method::Container &methods() const { return d->methods; }
    const Method::Container &sigs() const { return d->sigs; }
    const Method::Container &constructors() const { return d->constructors; }

    const Enum::Container &enums() const { return d->enums; }

    Kind kind() const { return d->kind; }

private:
    friend bool operator==(const MetaType &a, const MetaType &b) noexcept
    {
        return a.d == b.d;
    }

    friend bool operator!=(const MetaType &a, const MetaType &b) noexcept
    {
        return !(a == b);
    }

    static const MetaTypePrivate s_empty;
    const MetaTypePrivate *d = &s_empty;
};

struct UsingDeclaration {
    QAnyStringView alias;
    QAnyStringView original;

    bool isValid() const { return !alias.isEmpty() && !original.isEmpty(); }
private:
    friend bool comparesEqual(const UsingDeclaration &a, const UsingDeclaration &b) noexcept
    {
        return std::tie(a.alias, a.original) == std::tie(b.alias, b.original);
    }

    friend Qt::strong_ordering compareThreeWay(
            const UsingDeclaration &a, const UsingDeclaration &b) noexcept
    {
        return a.alias != b.alias
                ? compareThreeWay(a.alias, b.alias)
                : compareThreeWay(a.original, b.original);
    }
    Q_DECLARE_STRONGLY_ORDERED(UsingDeclaration);
};

class MetaTypesJsonProcessor
{
public:
    static QList<QAnyStringView> namespaces(const MetaType &classDef);

    MetaTypesJsonProcessor(bool privateIncludes) : m_privateIncludes(privateIncludes) {}

    bool processTypes(const QStringList &files);

    bool processForeignTypes(const QString &foreignTypesFile);
    bool processForeignTypes(const QStringList &foreignTypesFiles);

    void postProcessTypes();
    void postProcessForeignTypes();

    QVector<MetaType> types() const { return m_types; }
    QVector<MetaType> foreignTypes() const { return m_foreignTypes; }
    QList<QAnyStringView> referencedTypes() const { return m_referencedTypes; }
    QList<UsingDeclaration> usingDeclarations() const { return m_usingDeclarations; }
    QList<QString> includes() const { return m_includes; }

    QString extractRegisteredTypes() const;

private:
    enum RegistrationMode {
        NoRegistration,
        ObjectRegistration,
        GadgetRegistration,
        NamespaceRegistration
    };

    struct PreProcessResult {
        QList<QAnyStringView> primitiveAliases;
        UsingDeclaration usingDeclaration;
        QAnyStringView foreignPrimitive;
        RegistrationMode mode;
    };

    enum class PopulateMode { No, Yes };
    static PreProcessResult preProcess(const MetaType &classDef, PopulateMode populateMode);
    void addRelatedTypes();

    void sortTypes(QVector<MetaType> &types);
    QString resolvedInclude(QAnyStringView include);
    void processTypes(const QCborMap &types);
    void processForeignTypes(const QCborMap &types);

    bool isPrimitive(QAnyStringView type) const
    {
        return std::binary_search(m_primitiveTypes.begin(), m_primitiveTypes.end(), type);
    }

    QList<QString> m_includes;
    QList<QAnyStringView> m_referencedTypes;
    QList<QAnyStringView> m_primitiveTypes;
    QList<UsingDeclaration> m_usingDeclarations;
    QVector<MetaType> m_types;
    QVector<MetaType> m_foreignTypes;
    QDuplicateTracker<QString> m_seenMetaTypesFiles;
    bool m_privateIncludes = false;
};

QT_END_NAMESPACE

#endif // METATYPESJSONPROCESSOR_P_H
