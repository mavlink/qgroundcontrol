// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLJSLEXER_P_H
#define QQMLJSLEXER_P_H

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

#include <private/qqmljsglobal_p.h>
#include <private/qqmljsgrammar_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qstack.h>

QT_BEGIN_NAMESPACE

class QDebug;

namespace QQmlJS {

class Engine;
struct DiagnosticMessage;
class Directives;

class QML_PARSER_EXPORT Lexer: public QQmlJSGrammar
{
public:
    enum {
        T_ABSTRACT = T_RESERVED_WORD,
        T_BOOLEAN = T_RESERVED_WORD,
        T_BYTE = T_RESERVED_WORD,
        T_CHAR = T_RESERVED_WORD,
        T_DOUBLE = T_RESERVED_WORD,
        T_FLOAT = T_RESERVED_WORD,
        T_GOTO = T_RESERVED_WORD,
        T_IMPLEMENTS = T_RESERVED_WORD,
        T_INT = T_RESERVED_WORD,
        T_INTERFACE = T_RESERVED_WORD,
        T_LONG = T_RESERVED_WORD,
        T_NATIVE = T_RESERVED_WORD,
        T_PACKAGE = T_RESERVED_WORD,
        T_PRIVATE = T_RESERVED_WORD,
        T_PROTECTED = T_RESERVED_WORD,
        T_SHORT = T_RESERVED_WORD,
        T_SYNCHRONIZED = T_RESERVED_WORD,
        T_THROWS = T_RESERVED_WORD,
        T_TRANSIENT = T_RESERVED_WORD,
        T_VOLATILE = T_RESERVED_WORD
    };

    enum Error {
        NoError,
        IllegalCharacter,
        IllegalNumber,
        UnclosedStringLiteral,
        IllegalEscapeSequence,
        IllegalUnicodeEscapeSequence,
        UnclosedComment,
        IllegalExponentIndicator,
        IllegalIdentifier,
        IllegalHexadecimalEscapeSequence
    };

    enum RegExpBodyPrefix {
        NoPrefix,
        EqualPrefix
    };

    enum RegExpFlag {
        RegExp_Global     = 0x01,
        RegExp_IgnoreCase = 0x02,
        RegExp_Multiline  = 0x04,
        RegExp_Unicode    = 0x08,
        RegExp_Sticky     = 0x10
    };

    enum ParseModeFlags {
        QmlMode = 0x1,
        YieldIsKeyword = 0x2,
        StaticIsKeyword = 0x4
    };

    enum class ImportState {
        SawImport,
        NoQmlImport
    };

    enum class LexMode { WholeCode, LineByLine };

    enum class CodeContinuation { Reset, Continue };

public:
    Lexer(Engine *engine, LexMode lexMode = LexMode::WholeCode);

    bool qmlMode() const;
    bool yieldIsKeyWord() const { return _state.generatorLevel != 0; }
    void setStaticIsKeyword(bool b) { _staticIsKeyword = b; }

    QString code() const;
    void setCode(const QString &code, int lineno, bool qmlMode = true,
                 CodeContinuation codeContinuation = CodeContinuation::Reset);

    int lex();

    bool scanRegExp(RegExpBodyPrefix prefix = NoPrefix);
    bool scanDirectives(Directives *directives, DiagnosticMessage *error);

    int regExpFlags() const { return _state.patternFlags; }
    QString regExpPattern() const { return _tokenText; }

    int tokenKind() const { return _state.tokenKind; }
    int tokenOffset() const { return _currentOffset + _tokenStartPtr - _code.unicode(); }
    int tokenLength() const { return _tokenLength; }

    int tokenStartLine() const { return _tokenLine; }
    int tokenStartColumn() const { return _tokenColumn; }

    inline QStringView tokenSpell() const { return _tokenSpell; }
    inline QStringView rawString() const { return _rawString; }
    double tokenValue() const { return _state.tokenValue; }
    QString tokenText() const;

    Error errorCode() const;
    QString errorMessage() const;

    std::optional<DiagnosticMessage> illegalFileLengthError() const;

    bool canInsertAutomaticSemicolon(int token) const;

    enum ParenthesesState {
        IgnoreParentheses,
        CountParentheses,
        BalancedParentheses
    };

    enum class CommentState { NoComment, HadComment, InMultilineComment };

    void enterGeneratorBody() { ++_state.generatorLevel; }
    void leaveGeneratorBody() { --_state.generatorLevel; }

    struct State
    {
        Error errorCode = NoError;

        QChar currentChar = u'\n';
        double tokenValue = 0;

        // parentheses state
        ParenthesesState parenthesesState = IgnoreParentheses;
        int parenthesesCount = 0;

        // template string stack
        QStack<int> outerTemplateBraceCount;
        int bracesCount = -1;

        int stackToken = -1;

        int patternFlags = 0;
        int tokenKind = 0;
        ImportState importState = ImportState::NoQmlImport;

        bool validTokenText = false;
        bool prohibitAutomaticSemicolon = false;
        bool restrictedKeyword = false;
        bool terminator = false;
        bool followsClosingBrace = false;
        bool delimited = true;
        bool handlingDirectives = false;
        CommentState comments = CommentState::NoComment;
        int generatorLevel = 0;

        friend bool operator==(State const &s1, State const &s2)
        {
            if (s1.errorCode != s2.errorCode)
                return false;
            if (s1.currentChar != s2.currentChar)
                return false;
            if (s1.tokenValue != s2.tokenValue)
                return false;
            if (s1.parenthesesState != s2.parenthesesState)
                return false;
            if (s1.parenthesesCount != s2.parenthesesCount)
                return false;
            if (s1.outerTemplateBraceCount != s2.outerTemplateBraceCount)
                return false;
            if (s1.bracesCount != s2.bracesCount)
                return false;
            if (s1.stackToken != s2.stackToken)
                return false;
            if (s1.patternFlags != s2.patternFlags)
                return false;
            if (s1.tokenKind != s2.tokenKind)
                return false;
            if (s1.importState != s2.importState)
                return false;
            if (s1.validTokenText != s2.validTokenText)
                return false;
            if (s1.prohibitAutomaticSemicolon != s2.prohibitAutomaticSemicolon)
                return false;
            if (s1.restrictedKeyword != s2.restrictedKeyword)
                return false;
            if (s1.terminator != s2.terminator)
                return false;
            if (s1.followsClosingBrace != s2.followsClosingBrace)
                return false;
            if (s1.delimited != s2.delimited)
                return false;
            if (s1.handlingDirectives != s2.handlingDirectives)
                return false;
            if (s1.generatorLevel != s2.generatorLevel)
                return false;
            return true;
        }

        friend bool operator!=(State const &s1, State const &s2) { return !(s1 == s2); }

        friend QML_PARSER_EXPORT QDebug operator<<(QDebug dbg, State const &s);
    };

    const State &state() const;
    void setState(const State &state);

protected:
    static int classify(QStringView s, int parseModeFlags);

private:
    int parseModeFlags() const;
    bool prevTerminator() const;
    bool followsClosingBrace() const;
    inline void scanChar();
    inline QChar peekChar();
    int scanToken();
    int scanNumber(QChar ch);
    int scanVersionNumber(QChar ch);
    enum ScanStringMode : char16_t {
        SingleQuote = '\'',
        DoubleQuote = '"',
        TemplateHead = '`',
        TemplateContinuation = 0
    };
    int scanString(ScanStringMode mode);

    bool isLineTerminator() const;
    unsigned isLineTerminatorSequence() const;
    static bool isIdentLetter(QChar c);
    static bool isDecimalDigit(ushort c);
    static bool isHexDigit(QChar c);
    static bool isOctalDigit(ushort c);

    void syncProhibitAutomaticSemicolon();
    uint decodeUnicodeEscapeCharacter(bool *ok);
    QChar decodeHexEscapeCharacter(bool *ok);

    friend QML_PARSER_EXPORT QDebug operator<<(QDebug dbg, const Lexer &l);

private:
    Engine *_engine;

    LexMode _lexMode = LexMode::WholeCode;
    QString _code;
    const QChar *_endPtr;
    bool _qmlMode;
    bool _staticIsKeyword = false;

    bool _skipLinefeed = false;

    int _currentLineNumber = 0;
    int _currentColumnNumber = 0;
    int _currentOffset = 0;

    int _tokenLength = 0;
    int _tokenLine = 0;
    int _tokenColumn = 0;

    QString _tokenText;
    QString _errorMessage;
    QStringView _tokenSpell;
    QStringView _rawString;

    const QChar *_codePtr = nullptr;
    const QChar *_tokenStartPtr = nullptr;

    State _state;
};

} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // LEXER_H
