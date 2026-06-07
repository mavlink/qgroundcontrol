// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4BYTECODEHANDLER_P_H
#define QV4BYTECODEHANDLER_P_H

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

#include <private/qtqmlcompilerglobal_p.h>
#include <private/qv4instr_moth_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Moth {

#define BYTECODE_HANDLER_DEFINE_ARGS(nargs, ...) \
    MOTH_EXPAND_FOR_MSVC(BYTECODE_HANDLER_DEFINE_ARGS##nargs(__VA_ARGS__))

#define BYTECODE_HANDLER_DEFINE_ARGS0()
#define BYTECODE_HANDLER_DEFINE_ARGS1(arg) \
    int arg
#define BYTECODE_HANDLER_DEFINE_ARGS2(arg1, arg2) \
    int arg1, \
    int arg2
#define BYTECODE_HANDLER_DEFINE_ARGS3(arg1, arg2, arg3) \
    int arg1, \
    int arg2, \
    int arg3
#define BYTECODE_HANDLER_DEFINE_ARGS4(arg1, arg2, arg3, arg4) \
    int arg1, \
    int arg2, \
    int arg3, \
    int arg4
#define BYTECODE_HANDLER_DEFINE_ARGS5(arg1, arg2, arg3, arg4, arg5) \
    int arg1, \
    int arg2, \
    int arg3, \
    int arg4, \
    int arg5

#define BYTECODE_HANDLER_DEFINE_VIRTUAL_BYTECODE_HANDLER_INSTRUCTION(name, nargs, ...) \
    virtual void generate_##name( \
    BYTECODE_HANDLER_DEFINE_ARGS(nargs, __VA_ARGS__) \
    ) = 0;

#define BYTECODE_HANDLER_DEFINE_VIRTUAL_BYTECODE_HANDLER(instr) \
    INSTR_##instr(BYTECODE_HANDLER_DEFINE_VIRTUAL_BYTECODE_HANDLER)

class Q_QML_COMPILER_EXPORT ByteCodeHandler
{
    Q_DISABLE_COPY_MOVE(ByteCodeHandler)
public:
    ByteCodeHandler() = default;
    virtual ~ByteCodeHandler();

    void decode(const char *code, uint len);
    void reset() { _currentOffset = _nextOffset = 0; }

    int currentInstructionOffset() const { return _currentOffset; }
    int nextInstructionOffset() const { return _nextOffset; }
    int absoluteOffset(int relativeOffset) const
    { return nextInstructionOffset() + relativeOffset; }

protected:
    FOR_EACH_MOTH_INSTR(BYTECODE_HANDLER_DEFINE_VIRTUAL_BYTECODE_HANDLER)

    enum Verdict { ProcessInstruction, SkipInstruction };
    virtual Verdict startInstruction(Moth::Instr::Type instr) = 0;
    virtual void endInstruction(Moth::Instr::Type instr) = 0;

private:
    int _currentOffset = 0;
    int _nextOffset = 0;
};

} // Moth namespace
} // QV4 namespace

QT_END_NAMESPACE

#endif // QV4BYTECODEHANDLER_P_H
