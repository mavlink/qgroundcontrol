// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLSA_H
#define QQMLSA_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the qmllint plugin API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtQmlCompiler/qqmlsaconstants.h>
#include <QtQmlCompiler/qqmljsloggingutils.h>

#include <QtQmlCompiler/qtqmlcompilerexports.h>

#include <QtCore/qhash.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qplugin.h>
#include <QtQmlCompiler/qqmlsasourcelocation.h>

#include <unordered_map>

QT_BEGIN_NAMESPACE

namespace QQmlSA {

class BindingPrivate;
class BindingsPrivate;
class Element;
class ElementPass;
class FixSuggestion;
class FixSuggestionPrivate;
class GenericPassPrivate;
class MethodPrivate;
class MethodsPrivate;
class PassManager;
class PassManagerPrivate;
class PropertyPass;
class PropertyPrivate;
struct PropertyPassInfo;

enum class MethodType { Signal, Slot, Method, StaticMethod };
enum class AccessSemantics { Reference, Value, None, Sequence };

class Q_QMLCOMPILER_EXPORT Binding
{
    Q_DECLARE_PRIVATE(Binding)

public:
    class Q_QMLCOMPILER_EXPORT Bindings
    {
        Q_DECLARE_PRIVATE(Bindings)

    public:
        Bindings();
        Bindings(const Bindings &);
        ~Bindings();

        QMultiHash<QString, Binding>::const_iterator begin() const { return constBegin(); }
        QMultiHash<QString, Binding>::const_iterator end() const { return constEnd(); }
        QMultiHash<QString, Binding>::const_iterator constBegin() const;
        QMultiHash<QString, Binding>::const_iterator constEnd() const;

    private:
        std::unique_ptr<BindingsPrivate> d_ptr;
    };

    Binding();
    Binding(const Binding &);
    Binding(Binding &&) noexcept;
    Binding &operator=(const Binding &);
    Binding &operator=(Binding &&) noexcept;
    ~Binding();

    Element groupType() const;
    Element bindingScope() const;
    BindingType bindingType() const;
    QString stringValue() const;
    QString propertyName() const;
    bool isAttached() const;
    Element attachedType() const;

#if QT_DEPRECATED_SINCE(6, 9)
    QT_DEPRECATED_X("Use attachedType()")
    Element attachingType() const;
#endif

    QQmlSA::SourceLocation sourceLocation() const;
    double numberValue() const;
    ScriptBindingKind scriptKind() const;
    bool hasObject() const;
    Element objectType() const;
    bool hasUndefinedScriptValue() const;
    bool hasFunctionScriptValue() const;

    friend bool operator==(const Binding &lhs, const Binding &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }
    friend bool operator!=(const Binding &lhs, const Binding &rhs)
    {
        return !operatorEqualsImpl(lhs, rhs);
    }

    static bool isLiteralBinding(BindingType);

private:
    static bool operatorEqualsImpl(const Binding &, const Binding &);

    std::unique_ptr<BindingPrivate> d_ptr;
};

class Q_QMLCOMPILER_EXPORT Method
{
    Q_DECLARE_PRIVATE(Method)

public:
    class Q_QMLCOMPILER_EXPORT Methods
    {
        Q_DECLARE_PRIVATE(Methods)

    public:
        Methods();
        Methods(const Methods &);
        ~Methods();

        QMultiHash<QString, Method>::const_iterator begin() const { return constBegin(); }
        QMultiHash<QString, Method>::const_iterator end() const { return constEnd(); }
        QMultiHash<QString, Method>::const_iterator constBegin() const;
        QMultiHash<QString, Method>::const_iterator constEnd() const;

    private:
        std::unique_ptr<MethodsPrivate> d_ptr;
    };

    Method();
    Method(const Method &);
    Method(Method &&) noexcept;
    Method &operator=(const Method &);
    Method &operator=(Method &&) noexcept;
    ~Method();

    QString methodName() const;
    QQmlSA::SourceLocation sourceLocation() const;
    MethodType methodType() const;

    friend bool operator==(const Method &lhs, const Method &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }
    friend bool operator!=(const Method &lhs, const Method &rhs)
    {
        return !operatorEqualsImpl(lhs, rhs);
    }

private:
    static bool operatorEqualsImpl(const Method &, const Method &);

    std::unique_ptr<MethodPrivate> d_ptr;
};

class Q_QMLCOMPILER_EXPORT Property
{
    Q_DECLARE_PRIVATE(Property)

public:
    Property();
    Property(const Property &);
    Property(Property &&) noexcept;
    Property &operator=(const Property &);
    Property &operator=(Property &&) noexcept;
    ~Property();

    QString typeName() const;
    bool isValid() const;
    bool isReadonly() const;
    QQmlSA::Element type() const;

    friend bool operator==(const Property &lhs, const Property &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }

    friend bool operator!=(const Property &lhs, const Property &rhs)
    {
        return !operatorEqualsImpl(lhs, rhs);
    }

private:
    static bool operatorEqualsImpl(const Property &, const Property &);

    std::unique_ptr<PropertyPrivate> d_ptr;
};

class Q_QMLCOMPILER_EXPORT Element
{
    friend class QT_PREPEND_NAMESPACE(QQmlJSScope);

public:
    Element();
    Element(const Element &);
    Element(Element &&other) noexcept
    {
        memcpy(m_data, other.m_data, sizeofElement);
        memset(other.m_data, 0, sizeofElement);
    }
    Element &operator=(const Element &);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(Element)
    ~Element();

    ScopeType scopeType() const;
    Element baseType() const;
    QString baseTypeName() const;
    Element parentScope() const;
    bool inherits(const Element &) const;
    bool isFileRootComponent() const;

    bool isNull() const;
    QString internalId() const;
    AccessSemantics accessSemantics() const;
    bool isComposite() const;

    bool hasProperty(const QString &propertyName) const;
    bool hasOwnProperty(const QString &propertyName) const;
    Property property(const QString &propertyName) const;
    bool isPropertyRequired(const QString &propertyName) const;
    QString defaultPropertyName() const;

    bool hasMethod(const QString &methodName) const;
    Method::Methods ownMethods() const;

    QQmlSA::SourceLocation sourceLocation() const;
    QQmlSA::SourceLocation idSourceLocation() const;
    QString filePath() const;

    bool hasPropertyBindings(const QString &name) const;
    bool hasOwnPropertyBindings(const QString &propertyName) const;

    Binding::Bindings ownPropertyBindings() const;
    Binding::Bindings ownPropertyBindings(const QString &propertyName) const;
    QList<Binding> propertyBindings(const QString &propertyName) const;

    explicit operator bool() const;
    bool operator!() const;

    QString name() const;

    friend inline bool operator==(const QQmlSA::Element &lhs, const QQmlSA::Element &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }
    friend inline bool operator!=(const Element &lhs, const Element &rhs) { return !(lhs == rhs); }

    friend inline qsizetype qHash(const Element &key, qsizetype seed = 0) noexcept
    {
        return qHashImpl(key, seed);
    }

private:
    static bool operatorEqualsImpl(const Element &, const Element &);
    static qsizetype qHashImpl(const Element &key, qsizetype seed) noexcept;

    static constexpr qsizetype sizeofElement = 2 * sizeof(QSharedPointer<int>);
    alignas(QSharedPointer<int>) char m_data[sizeofElement];

    void swap(Element &other) noexcept
    {
        char t[sizeofElement];
        memcpy(t, m_data, sizeofElement);
        memcpy(m_data, other.m_data, sizeofElement);
        memcpy(other.m_data, t, sizeofElement);
    }
    friend void swap(Element &lhs, Element &rhs) noexcept { lhs.swap(rhs); }
};

class Q_QMLCOMPILER_EXPORT GenericPass
{
    Q_DECLARE_PRIVATE(GenericPass)
    Q_DISABLE_COPY_MOVE(GenericPass)

public:
    GenericPass(PassManager *manager);
    virtual ~GenericPass();

    void emitWarning(QAnyStringView diagnostic, LoggerWarningId id);
    void emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                     QQmlSA::SourceLocation srcLocation);
    void emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                     QQmlSA::SourceLocation srcLocation, const QQmlSA::FixSuggestion &fix);

    Element resolveTypeInFileScope(QAnyStringView typeName);
    Element resolveAttachedInFileScope(QAnyStringView typeName);
    Element resolveType(QAnyStringView moduleName, QAnyStringView typeName); // #### TODO: revisions
    Element resolveBuiltinType(QAnyStringView typeName) const;
    Element resolveAttached(QAnyStringView moduleName, QAnyStringView typeName);
    Element resolveLiteralType(const Binding &binding);

    Element resolveIdToElement(QAnyStringView id, const Element &context);
    QString resolveElementToId(const Element &element, const Element &context);

    QString sourceCode(QQmlSA::SourceLocation location);

private:
    std::unique_ptr<GenericPassPrivate> d_ptr;
};

class Q_QMLCOMPILER_EXPORT PassManager
{
    Q_DISABLE_COPY_MOVE(PassManager)
    Q_DECLARE_PRIVATE(PassManager)

public:
    void registerElementPass(std::unique_ptr<ElementPass> pass);
    bool registerPropertyPass(std::shared_ptr<PropertyPass> pass, QAnyStringView moduleName,
                              QAnyStringView typeName,
                              QAnyStringView propertyName = QAnyStringView(),
                              bool allowInheritance = true);
    void analyze(const Element &root);

    bool hasImportedModule(QAnyStringView name) const;

    bool isCategoryEnabled(LoggerWarningId category) const;

    std::unordered_map<quint32, Binding> bindingsByLocation() const;

private:
    PassManager();
    ~PassManager();

    std::unique_ptr<PassManagerPrivate> d_ptr;
};

class Q_QMLCOMPILER_EXPORT LintPlugin
{
public:
    LintPlugin() = default;
    virtual ~LintPlugin() = default;

    Q_DISABLE_COPY_MOVE(LintPlugin)

    virtual void registerPasses(PassManager *manager, const Element &rootElement) = 0;
};

class Q_QMLCOMPILER_EXPORT PropertyPass : public GenericPass
{
public:
    PropertyPass(PassManager *manager);

    virtual void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                           const QQmlSA::Binding &binding, const QQmlSA::Element &bindingScope,
                           const QQmlSA::Element &value);
    virtual void onRead(const QQmlSA::Element &element, const QString &propertyName,
                        const QQmlSA::Element &readScope, QQmlSA::SourceLocation location);
    virtual void onCall(const QQmlSA::Element &element, const QString &propertyName,
                        const QQmlSA::Element &readScope, QQmlSA::SourceLocation location);
    virtual void onWrite(const QQmlSA::Element &element, const QString &propertyName,
                         const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                         QQmlSA::SourceLocation location);
};

class Q_QMLCOMPILER_EXPORT ElementPass : public GenericPass
{
public:
    ElementPass(PassManager *manager) : GenericPass(manager) { }

    virtual bool shouldRun(const Element &element);
    virtual void run(const Element &element) = 0;
};

class Q_QMLCOMPILER_EXPORT DebugElementPass : public ElementPass
{
    void run(const Element &element) override;
};

class Q_QMLCOMPILER_EXPORT DebugPropertyPass : public QQmlSA::PropertyPass
{
public:
    DebugPropertyPass(QQmlSA::PassManager *manager);

    void onRead(const QQmlSA::Element &element, const QString &propertyName,
                const QQmlSA::Element &readScope, QQmlSA::SourceLocation location) override;
    void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                   const QQmlSA::Binding &binding, const QQmlSA::Element &bindingScope,
                   const QQmlSA::Element &value) override;
    void onWrite(const QQmlSA::Element &element, const QString &propertyName,
                 const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                 QQmlSA::SourceLocation location) override;
};

class Q_QMLCOMPILER_EXPORT FixSuggestion
{
    Q_DECLARE_PRIVATE(FixSuggestion)

public:
    FixSuggestion(const QString &fixDescription, const QQmlSA::SourceLocation &location,
                  const QString &replacement = QString());
    FixSuggestion(const FixSuggestion &);
    FixSuggestion(FixSuggestion &&) noexcept;
    FixSuggestion &operator=(const FixSuggestion &);
    FixSuggestion &operator=(FixSuggestion &&) noexcept;
    ~FixSuggestion();

    QString fixDescription() const;
    QQmlSA::SourceLocation location() const;
    QString replacement() const;

    void setFileName(const QString &);
    QString fileName() const;

    void setHint(const QString &);
    QString hint() const;

    void setAutoApplicable(bool autoApplicable = true);
    bool isAutoApplicable() const;

    friend bool operator==(const FixSuggestion &lhs, const FixSuggestion &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }

    friend bool operator!=(const FixSuggestion &lhs, const FixSuggestion &rhs)
    {
        return !operatorEqualsImpl(lhs, rhs);
    }

private:
    static bool operatorEqualsImpl(const FixSuggestion &, const FixSuggestion &);

    std::unique_ptr<FixSuggestionPrivate> d_ptr;
};

} // namespace QQmlSA

#define QmlLintPluginInterface_iid "org.qt-project.Qt.Qml.SA.LintPlugin/1.0"

Q_DECLARE_INTERFACE(QQmlSA::LintPlugin, QmlLintPluginInterface_iid)

QT_END_NAMESPACE

#endif // QQMLSA_H
