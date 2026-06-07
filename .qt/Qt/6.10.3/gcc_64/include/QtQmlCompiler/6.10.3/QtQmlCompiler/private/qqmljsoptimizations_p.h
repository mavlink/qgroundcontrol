// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSOPTIMIZATIONS_P_H
#define QQMLJSOPTIMIZATIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qqmljscompilepass_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_EXPORT QQmlJSOptimizations : public QQmlJSCompilePass
{
public:
    using Conversions = QSet<int>;

    QQmlJSOptimizations(const QV4::Compiler::JSUnitGenerator *unitGenerator,
                        const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger,
                        const BasicBlocks &basicBlocks, const InstructionAnnotations &annotations,
                        QList<ObjectOrArrayDefinition> objectAndArrayDefinitions)
        : QQmlJSCompilePass(unitGenerator, typeResolver, logger, basicBlocks, annotations),
          m_objectAndArrayDefinitions{ std::move(objectAndArrayDefinitions) }
    {
    }

    ~QQmlJSOptimizations() = default;

    BlocksAndAnnotations run(const Function *function);

private:
    struct RegisterAccess
    {
        QList<QQmlJSRegisterContent> trackedTypes;
        QHash<int, QQmlJSRegisterContent> typeReaders;
        QHash<int, Conversions> registerReadersAndConversions;
        int trackedRegister;
    };

    QV4::Moth::ByteCodeHandler::Verdict startInstruction(QV4::Moth::Instr::Type) override
    {
        return ProcessInstruction;
    }
    void endInstruction(QV4::Moth::Instr::Type) override { }

    void populateBasicBlocks();
    void populateReaderLocations();
    void adjustTypes();
    bool canMove(int instructionOffset, const RegisterAccess &access) const;

    void removeReadsFromErasedInstructions(const QFlatMap<int, InstructionAnnotation>::const_iterator &it);
    void removeDeadStoresUntilStable();
    bool eraseDeadStore(const InstructionAnnotations::iterator &it, bool &erasedReaders);

    QHash<int, RegisterAccess> m_readerLocations;
    QList<ObjectOrArrayDefinition> m_objectAndArrayDefinitions;
};

QT_END_NAMESPACE

#endif // QQMLJSOPTIMIZATIONS_P_H
