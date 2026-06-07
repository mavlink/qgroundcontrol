// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSTYPEPROPAGATOR_P_H
#define QQMLJSTYPEPROPAGATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qqmljsast_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljscompilepass_p.h>
#include <private/qqmljscontextproperties_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlSA {
class PassManager;
};

struct Q_QMLCOMPILER_EXPORT QQmlJSTypePropagator : public QQmlJSCompilePass
{
    QQmlJSTypePropagator(const QV4::Compiler::JSUnitGenerator *unitGenerator,
                         const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger,
                         const BasicBlocks &basicBlocks = {},
                         const InstructionAnnotations &annotations = {},
                         QQmlSA::PassManager *passManager = nullptr,
                         const QQmlJS::ContextProperties &knownContextProperties = {});

    BlocksAndAnnotations run(const Function *m_function);

    void generate_Ret() override;
    void generate_Debug() override;
    void generate_LoadConst(int index) override;
    void generate_LoadZero() override;
    void generate_LoadTrue() override;
    void generate_LoadFalse() override;
    void generate_LoadNull() override;
    void generate_LoadUndefined() override;
    void generate_LoadInt(int value) override;
    void generate_MoveConst(int constIndex, int destTemp) override;
    void generate_LoadReg(int reg) override;
    void generate_StoreReg(int reg) override;
    void generate_MoveReg(int srcReg, int destReg) override;
    void generate_LoadImport(int index) override;
    void generate_LoadLocal(int index) override;
    void generate_StoreLocal(int index) override;
    void generate_LoadScopedLocal(int scope, int index) override;
    void generate_StoreScopedLocal(int scope, int index) override;
    void generate_LoadRuntimeString(int stringId) override;
    void generate_MoveRegExp(int regExpId, int destReg) override;
    void generate_LoadClosure(int value) override;
    void generate_LoadName(int nameIndex) override;
    void generate_LoadGlobalLookup(int index) override;
    void generate_LoadQmlContextPropertyLookup(int index) override;
    void generate_StoreNameCommon(int nameIndex);
    void generate_StoreNameSloppy(int nameIndex) override;
    void generate_StoreNameStrict(int name) override;
    void generate_LoadElement(int base) override;
    void generate_StoreElement(int base, int index) override;
    void generate_LoadProperty(int nameIndex) override;
    void generate_LoadOptionalProperty(int name, int offset) override;
    void generate_GetLookup(int index) override;
    void generate_GetOptionalLookup(int index, int offset) override;
    void generate_StoreProperty(int name, int base) override;
    void generate_SetLookup(int index, int base) override;
    void generate_LoadSuperProperty(int property) override;
    void generate_StoreSuperProperty(int property) override;
    void generate_Yield() override;
    void generate_YieldStar() override;
    void generate_Resume(int) override;

    void generate_CallValue(int name, int argc, int argv) override;
    void generate_CallWithReceiver(int name, int thisObject, int argc, int argv) override;
    void generate_CallProperty(int name, int base, int argc, int argv) override;
    void generate_CallPropertyLookup(int lookupIndex, int base, int argc, int argv) override;
    void generate_CallName(int name, int argc, int argv) override;
    void generate_CallPossiblyDirectEval(int argc, int argv) override;
    void generate_CallGlobalLookup(int index, int argc, int argv) override;
    void generate_CallQmlContextPropertyLookup(int index, int argc, int argv) override;
    void generate_CallWithSpread(int func, int thisObject, int argc, int argv) override;
    void generate_TailCall(int func, int thisObject, int argc, int argv) override;
    void generate_Construct(int func, int argc, int argv) override;
    void generate_ConstructWithSpread(int func, int argc, int argv) override;
    void generate_SetUnwindHandler(int offset) override;
    void generate_UnwindDispatch() override;
    void generate_UnwindToLabel(int level, int offset) override;
    void generate_DeadTemporalZoneCheck(int name) override;
    void generate_ThrowException() override;
    void generate_GetException() override;
    void generate_SetException() override;
    void generate_CreateCallContext() override;
    void generate_PushCatchContext(int index, int name) override;
    void generate_PushWithContext() override;
    void generate_PushBlockContext(int index) override;
    void generate_CloneBlockContext() override;
    void generate_PushScriptContext(int index) override;
    void generate_PopScriptContext() override;
    void generate_PopContext() override;
    void generate_GetIterator(int iterator) override;
    void generate_IteratorNext(int value, int offset) override;
    void generate_IteratorNextForYieldStar(int iterator, int object, int offset) override;
    void generate_IteratorClose() override;
    void generate_DestructureRestElement() override;
    void generate_DeleteProperty(int base, int index) override;
    void generate_DeleteName(int name) override;
    void generate_TypeofName(int name) override;
    void generate_TypeofValue() override;
    void generate_DeclareVar(int varName, int isDeletable) override;
    void generate_DefineArray(int argc, int args) override;
    void generate_DefineObjectLiteral(int internalClassId, int argc, int args) override;
    void generate_CreateClass(int classIndex, int heritage, int computedNames) override;
    void generate_CreateMappedArgumentsObject() override;
    void generate_CreateUnmappedArgumentsObject() override;
    void generate_CreateRestParameter(int argIndex) override;
    void generate_ConvertThisToObject() override;
    void generate_LoadSuperConstructor() override;
    void generate_ToObject() override;
    void generate_Jump(int offset) override;
    void generate_JumpTrue(int offset) override;
    void generate_JumpFalse(int offset) override;
    void generate_JumpNoException(int offset) override;
    void generate_JumpNotUndefined(int offset) override;
    void generate_CheckException() override;
    void generate_CmpEqNull() override;
    void generate_CmpNeNull() override;
    void generate_CmpEqInt(int lhsConst) override;
    void generate_CmpNeInt(int lhs) override;
    void generate_CmpEq(int lhs) override;
    void generate_CmpNe(int lhs) override;
    void generate_CmpGt(int lhs) override;
    void generate_CmpGe(int lhs) override;
    void generate_CmpLt(int lhs) override;
    void generate_CmpLe(int lhs) override;
    void generate_CmpStrictEqual(int lhs) override;
    void generate_CmpStrictNotEqual(int lhs) override;
    void generate_CmpIn(int lhs) override;
    void generate_CmpInstanceOf(int lhs) override;
    void generate_As(int lhs) override;
    void generate_UNot() override;
    void generate_UPlus() override;
    void generate_UMinus() override;
    void generate_UCompl() override;
    void generate_Increment() override;
    void generate_Decrement() override;
    void generate_Add(int lhs) override;
    void generate_BitAnd(int lhs) override;
    void generate_BitOr(int lhs) override;
    void generate_BitXor(int lhs) override;
    void generate_UShr(int lhs) override;
    void generate_Shr(int lhs) override;
    void generate_Shl(int lhs) override;
    void generate_BitAndConst(int rhsConst) override;
    void generate_BitOrConst(int rhsConst) override;
    void generate_BitXorConst(int rhsConst) override;
    void generate_UShrConst(int rhsConst) override;
    void generate_ShrConst(int rhs) override;
    void generate_ShlConst(int rhs) override;
    void generate_Exp(int lhs) override;
    void generate_Mul(int lhs) override;
    void generate_Div(int lhs) override;
    void generate_Mod(int lhs) override;
    void generate_Sub(int lhs) override;
    void generate_InitializeBlockDeadTemporalZone(int firstReg, int count) override;
    void generate_ThrowOnNullOrUndefined() override;
    void generate_GetTemplateObject(int index) override;

    bool checkForEnumProblems(QQmlJSRegisterContent base, const QString &propertyName);

    Verdict startInstruction(QV4::Moth::Instr::Type instr) override;
    void endInstruction(QV4::Moth::Instr::Type instr) override;

private:
    struct ExpectedRegisterState
    {
        int originatingOffset = 0;
        VirtualRegisters registers;
    };

    struct PassState : QQmlJSCompilePass::State
    {
        InstructionAnnotations annotations;
        QSet<int> jumpTargets;
        bool skipInstructionsUntilNextJumpTarget = false;
        bool needsMorePasses = false;
        bool instructionHasError = false;
    };

    void handleUnqualifiedAccess(const QString &name, bool isMethod) const;
    void checkDeprecated(QQmlJSScope::ConstPtr scope, const QString &name, bool isMethod) const;
    bool isCallingProperty(QQmlJSScope::ConstPtr scope, const QString &name) const;

    enum PropertyResolution {
        PropertyMissing,
        PropertyTypeUnresolved,
        PropertyFullyResolved
    };

    PropertyResolution propertyResolution(QQmlJSScope::ConstPtr scope, const QString &type) const;

    void checkConversion(QQmlJSRegisterContent from, QQmlJSRegisterContent to);
    void generateUnaryArithmeticOperation(QQmlJSTypeResolver::UnaryOperator op);

    QQmlJSRegisterContent propagateBinaryOperation(QSOperator::Op op, int lhs);
    void generateBinaryArithmeticOperation(QSOperator::Op op, int lhs);
    void generateBinaryConstArithmeticOperation(QSOperator::Op op);

    void propagateCall(
            const QList<QQmlJSMetaMethod> &methods, int argc, int argv,
            QQmlJSRegisterContent scope);
    void propagateTranslationMethod_SAcheck(const QString &methodName);
    bool propagateTranslationMethod(const QList<QQmlJSMetaMethod> &methods, int argc, int argv);
    void propagateStringArgCall(QQmlJSRegisterContent base, int argv);
    bool propagateArrayMethod(const QString &name, int argc, int argv, QQmlJSRegisterContent valueType);
    void propagatePropertyLookup(
            const QString &name, int lookupIndex = QQmlJSRegisterContent::InvalidLookupIndex);
    void propagateScopeLookupCall(const QString &functionName, int argc, int argv);
    void saveRegisterStateForJump(int offset);
    bool canConvertFromTo(QQmlJSRegisterContent from, QQmlJSRegisterContent to);
    bool canConvertFromTo(QQmlJSRegisterContent from, const QQmlJSScope::ConstPtr &to);

    QString registerName(int registerIndex) const;

    QQmlJSRegisterContent checkedInputRegister(int reg);
    QQmlJSMetaMethod bestMatchForCall(const QList<QQmlJSMetaMethod> &methods, int argc, int argv,
                                      QStringList *errors);

    void setAccumulator(QQmlJSRegisterContent content);
    void setRegister(int index, QQmlJSRegisterContent content);
    void mergeRegister(int index, const VirtualRegister &a, const VirtualRegister &b);

    void addReadRegister(int index);
    void addReadRegister(int index, QQmlJSRegisterContent convertTo);
    void addReadRegister(int index, const QQmlJSScope::ConstPtr &convertTo);

    void addReadAccumulator()
    {
        addReadRegister(Accumulator);
    }

    void addReadAccumulator(QQmlJSRegisterContent convertTo)
    {
        addReadRegister(Accumulator, convertTo);
    }

    void addReadAccumulator(const QQmlJSScope::ConstPtr &convertTo)
    {
        addReadRegister(Accumulator, convertTo);
    }

    bool populatesAccumulator(QV4::Moth::Instr::Type instr) const;
    bool isNoop(QV4::Moth::Instr::Type instr) const;

    void recordEqualsNullType();
    void recordEqualsIntType();
    void recordEqualsType(int lhs);
    void recordCompareType(int lhs);

    // helper functions to deal with special cases in generate_ methods
    void generate_CallProperty_SCMath(const QString &name, int base, int arcg, int argv);
    void generate_CallProperty_SCconsole(const QString &name, int base, int argc, int argv);
    void generate_Construct_SCDate(const QQmlJSMetaMethod &ctor, int argc, int argv);
    void generate_Construct_SCArray(const QQmlJSMetaMethod &ctor, int argc, int argv);

    // helper functions to perform QQmlSA checks
    void generate_ret_SAcheck();
    void generate_LoadQmlContextPropertyLookup_SAcheck(const QString &name);
    void generate_StoreNameCommon_SAcheck(QQmlJSRegisterContent in, const QString &name);
    void propagatePropertyLookup_SAcheck(const QString &propertyName);
    void generate_StoreProperty_SAcheck(const QString &propertyName, QQmlJSRegisterContent callBase);
    void generate_callProperty_SAcheck(const QString &propertyName,
                                       const QQmlJSScope::ConstPtr &callBase);
    void propagateCall_SAcheck(const QQmlJSMetaMethod &method,
                               const QQmlJSScope::ConstPtr &baseType);
    void generate_GetOptionalLookup_SAcheck();

    bool handleImportNamespaceLookup(const QString &propertyName);
    void handleLookupError(const QString &propertyName);

    void addError(const QString &message)
    {
        QQmlJSCompilePass::addError(message);
        m_state.instructionHasError = true;
    }

    void setVarAccumulatorAndError()
    {
        setAccumulator(m_typeResolver->syntheticType(m_typeResolver->varType()));
        m_state.instructionHasError = true;
    }
    void warnAboutTypeCoercion(int lhs);

    QQmlJSRegisterContent m_returnType;
    QQmlSA::PassManager *m_passManager = nullptr;

    // Not part of the state, as the back jumps are the reason for running multiple passes
    QMultiHash<int, ExpectedRegisterState> m_jumpOriginRegisterStateByTargetInstructionOffset;

    InstructionAnnotations m_prevStateAnnotations;
    PassState m_state;
    QQmlJS::ContextProperties m_knownContextProperties;
};

QT_END_NAMESPACE

#endif // QQMLJSTYPEPROPAGATOR_P_H
