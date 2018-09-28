/** \file
    \brief Implementation of the token reader used to break the expression string up
    into tokens.

    <pre>
                __________                                 ____  ___
     _____  __ _\______   \_____ _______  ______ __________\   \/  /
    /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /
    |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \
    |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
          \/                     \/           \/     \/           \_/
    Copyright (C) 2016, Ingo Berg
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
    </pre>
    */

#include "mpTokenReader.h"

#include <cassert>

#include "mpParserBase.h"
#include "mpIValReader.h"
#include "mpIfThenElse.h"
#include "mpScriptTokens.h"
#include "mpOprtIndex.h"
#include "mpOprtMatrix.h"

MUP_NAMESPACE_START

//---------------------------------------------------------------------------
/** \brief Copy constructor.
    \sa Assign
    \throw nothrow
    */
    TokenReader::TokenReader(const TokenReader &a_Reader)
{
    Assign(a_Reader);
}

//---------------------------------------------------------------------------
/** \brief Assignement operator.
    \param a_Reader Object to copy to this token reader.
    \throw nothrow

    Self assignement will be suppressed otherwise #Assign is called.
    */
TokenReader& TokenReader::operator=(const TokenReader &a_Reader)
{
    if (&a_Reader != this)
        Assign(a_Reader);

    return *this;
}

//---------------------------------------------------------------------------
/** \brief Assign state of a token reader to this token reader.
    \param a_Reader Object from which the state should be copied.
    \throw nothrow
    */
void TokenReader::Assign(const TokenReader &obj)
{
    m_pParser = obj.m_pParser;
    m_sExpr = obj.m_sExpr;
    m_nPos = obj.m_nPos;
    m_nNumBra = obj.m_nNumBra;
    m_nNumIndex = obj.m_nNumIndex;
    m_nNumCurly = obj.m_nNumCurly;
    m_nNumIfElse = obj.m_nNumIfElse;
    m_nSynFlags = obj.m_nSynFlags;
    m_UsedVar = obj.m_UsedVar;
    m_pVarDef = obj.m_pVarDef;
    m_pPostOprtDef = obj.m_pPostOprtDef;
    m_pInfixOprtDef = obj.m_pInfixOprtDef;
    m_pOprtDef = obj.m_pOprtDef;
    m_pFunDef = obj.m_pFunDef;
    m_pConstDef = obj.m_pConstDef;
    m_pDynVarShadowValues = obj.m_pDynVarShadowValues;
    m_vTokens = obj.m_vTokens;

    // Reader klassen klonen 
    DeleteValReader();
    std::size_t i, iSize = obj.m_vValueReader.size();
    for (i = 0; i < iSize; ++i)
    {
        m_vValueReader.push_back(obj.m_vValueReader[i]->Clone(this));
    }
}

//---------------------------------------------------------------------------
/** \brief Constructor.

    Create a Token reader and bind it to a parser object.

    \pre [assert] a_pParser may not be nullptr
    \post #m_pParser==a_pParser
    \param a_pParent Parent parser object of the token reader.
    */
TokenReader::TokenReader(ParserXBase *a_pParent)
    :m_pParser(a_pParent)
    , m_sExpr()
    , m_nPos(0)
    , m_nNumBra(0)
    , m_nNumIndex(0)
    , m_nNumCurly(0)
    , m_nNumIfElse(0)
    , m_nSynFlags(0)
    , m_vTokens()
    , m_eLastTokCode(cmUNKNOWN)
    , m_pFunDef(nullptr)
    , m_pOprtDef(nullptr)
    , m_pInfixOprtDef(nullptr)
    , m_pPostOprtDef(nullptr)
    , m_pConstDef(nullptr)
    , m_pDynVarShadowValues(nullptr)
    , m_pVarDef(nullptr)
    , m_vValueReader()
    , m_UsedVar()
    , m_fZero(0)
{
    assert(m_pParser);
    SetParent(m_pParser);
}

//---------------------------------------------------------------------------
/** \brief Destructor (trivial).

    \throw nothrow
    */
TokenReader::~TokenReader()
{
    DeleteValReader();
}

//---------------------------------------------------------------------------
void TokenReader::DeleteValReader()
{
    int iSize = (int)m_vValueReader.size();
    for (int i = 0; i < iSize; ++i)
        delete m_vValueReader[i];

    m_vValueReader.clear();
}

//---------------------------------------------------------------------------
/** \brief Create instance of a ParserTokenReader identical with this
          and return its pointer.

          This is a factory method the calling function must take care of the object destruction.

          \return A new ParserTokenReader object.
          \throw nothrow
          */
TokenReader* TokenReader::Clone(ParserXBase *a_pParent) const
{
    std::unique_ptr<TokenReader> ptr(new TokenReader(*this));
    ptr->SetParent(a_pParent);
    return ptr.release();
}

//---------------------------------------------------------------------------
void TokenReader::AddValueReader(IValueReader *a_pReader)
{
    a_pReader->SetParent(this);
    m_vValueReader.push_back(a_pReader);
}

//---------------------------------------------------------------------------
void TokenReader::AddSynFlags(int flag)
{
    m_nSynFlags |= flag;
}

//---------------------------------------------------------------------------
const TokenReader::token_buf_type& TokenReader::GetTokens() const
{
    return m_vTokens;
}

//---------------------------------------------------------------------------
/** \brief Return the current position of the token reader in the formula string.

    \return #m_nPos
    \throw nothrow
    */
int TokenReader::GetPos() const
{
    return m_nPos;
}

//---------------------------------------------------------------------------
/** \brief Return a reference to the formula.

    \return #m_sExpr
    \throw nothrow
    */
const string_type& TokenReader::GetExpr() const
{
    return m_sExpr;
}

//---------------------------------------------------------------------------
/** \brief Return a map containing the used variables only. */
const var_maptype& TokenReader::GetUsedVar() const
{
    return m_UsedVar;
}

//---------------------------------------------------------------------------
/** \brief Initialize the token Reader.

    Sets the expression position index to zero and set Syntax flags to
    default for initial parsing.
    */
void TokenReader::SetExpr(const string_type &a_sExpr)
{
    m_sExpr = a_sExpr; // + string_type(_T(" "));
    ReInit();
}


//---------------------------------------------------------------------------
/** \brief Reset the token reader to the start of the formula.
    \post #m_nPos==0, #m_nSynFlags = noOPT | noBC | noPOSTOP | noSTR
    \throw nothrow
    \sa ESynCodes

    The syntax flags will be reset to a value appropriate for the
    start of a formula.
    */
void TokenReader::ReInit()
{
    m_nPos = 0;
    m_nNumBra = 0;
    m_nNumIndex = 0;
    m_nNumCurly = 0;
    m_nNumIfElse = 0;
    m_nSynFlags = noOPT | noBC | noCBC | noPFX | noCOMMA | noIO | noIC | noIF | noELSE;
    m_UsedVar.clear();
    m_eLastTokCode = cmUNKNOWN;
    m_vTokens.clear();
}

//---------------------------------------------------------------------------
const ptr_tok_type& TokenReader::Store(const ptr_tok_type &t, int token_pos)
{
    m_eLastTokCode = t->GetCode();
    t->SetExprPos(token_pos);
    m_vTokens.push_back(t);
    return t;
}

//---------------------------------------------------------------------------
void TokenReader::SkipCommentsAndWhitespaces()
{
    bool bSkip = true;
    while (m_nPos < (int)m_sExpr.length() && bSkip)
    {
        switch (m_sExpr[m_nPos])
        {
            // skip comments
        case  '#':
        {
            std::size_t i = m_sExpr.find_first_of('\n', m_nPos + 1);
            m_nPos = (i != string_type::npos) ? i : m_sExpr.length();
        }
        break;

        // skip whitespaces
        case ' ':
            ++m_nPos;
            break;

        default:
            bSkip = false;
        } // switch 
    } // while comment or whitespace
}

//---------------------------------------------------------------------------
/** \brief Read the next token from the string. */
ptr_tok_type TokenReader::ReadNextToken()
{
    assert(m_pParser);

    SkipCommentsAndWhitespaces();

    int token_pos = m_nPos;
    ptr_tok_type pTok;

    // Check for end of expression
    if (IsEOF(pTok))
        return Store(pTok, token_pos);

    if (IsNewline(pTok))
        return Store(pTok, token_pos);

    if (!(m_nSynFlags & noOPT) && IsOprt(pTok))
        return Store(pTok, token_pos); // Check for user defined binary operator

    if (!(m_nSynFlags & noIFX) && IsInfixOpTok(pTok))
        return Store(pTok, token_pos); // Check for unary operators

    if (IsValTok(pTok))
        return Store(pTok, token_pos); // Check for values / constant tokens

    if (IsBuiltIn(pTok))
        return Store(pTok, token_pos); // Check built in operators / tokens

    if (IsVarOrConstTok(pTok))
        return Store(pTok, token_pos); // Check for variable tokens

    if (IsFunTok(pTok))
        return Store(pTok, token_pos);

    if (!(m_nSynFlags & noPFX) && IsPostOpTok(pTok))
        return Store(pTok, token_pos); // Check for unary operators

    // 2.) We have found no token, maybe there is a token that we don't expect here.
    //     Again call the Identifier functions but this time only those we don't expect 
    //     to find.
    if ((m_nSynFlags & noOPT) && IsOprt(pTok))
        return Store(pTok, token_pos); // Check for user defined binary operator

    if ((m_nSynFlags & noIFX) && IsInfixOpTok(pTok))
        return Store(pTok, token_pos); // Check for unary operators

    if ((m_nSynFlags & noPFX) && IsPostOpTok(pTok))
        return Store(pTok, token_pos); // Check for unary operators
    // </ibg>

    // Now we are in trouble because there is something completely unknown....

    // Check the string for an undefined variable token. This is done 
    // only if a flag is set indicating to ignore undefined variables.
    // This is a way to conditionally avoid an error if undefined variables 
    // occur. The GetExprVar function must supress the error for undefined 
    // variables in order to collect all variable names including the 
    // undefined ones.
    if ((m_pParser->m_bIsQueryingExprVar || m_pParser->m_bAutoCreateVar) && IsUndefVarTok(pTok))
        return Store(pTok, token_pos);

    // Check for unknown token
    // 
    // !!! From this point on there is no exit without an exception possible...
    // 
    string_type sTok;
    int iEnd = ExtractToken(m_pParser->ValidNameChars(), sTok, m_nPos);

    ErrorContext err;
    err.Errc = ecUNASSIGNABLE_TOKEN;
    err.Expr = m_sExpr;
    err.Pos = m_nPos;

    if (iEnd != m_nPos)
        err.Ident = sTok;
    else
        err.Ident = m_sExpr.substr(m_nPos);

    throw ParserError(err);
}

//---------------------------------------------------------------------------
void TokenReader::SetParent(ParserXBase *a_pParent)
{
    m_pParser = a_pParent;
    m_pFunDef = &a_pParent->m_FunDef;
    m_pOprtDef = &a_pParent->m_OprtDef;
    m_pInfixOprtDef = &a_pParent->m_InfixOprtDef;
    m_pPostOprtDef = &a_pParent->m_PostOprtDef;
    m_pVarDef = &a_pParent->m_varDef;
    m_pConstDef = &a_pParent->m_valDef;
    m_pDynVarShadowValues = &a_pParent->m_valDynVarShadow;
}

//---------------------------------------------------------------------------
/** \brief Extract all characters that belong to a certain charset.
    \param a_szCharSet [in] Const char array of the characters allowed in the token.
    \param a_strTok [out]  The string that consists entirely of characters listed in a_szCharSet.
    \param a_iPos [in] Position in the string from where to start reading.
    \return The Position of the first character not listed in a_szCharSet.
    \throw nothrow
    */
int TokenReader::ExtractToken(const char_type *a_szCharSet,
    string_type &a_sTok,
    int a_iPos) const
{
    int iEnd = (int)m_sExpr.find_first_not_of(a_szCharSet, a_iPos);

    if (iEnd == (int)string_type::npos)
        iEnd = (int)m_sExpr.length();

    if (iEnd != a_iPos)
        a_sTok.assign(m_sExpr.begin() + a_iPos, m_sExpr.begin() + iEnd);

    return iEnd;
}

//---------------------------------------------------------------------------
/** \brief Check if a built in operator or other token can be found.
*/
bool TokenReader::IsBuiltIn(ptr_tok_type &a_Tok)
{
    const char_type **pOprtDef = m_pParser->GetOprtDef(),
        *szFormula = m_sExpr.c_str();
    int i;

    try
    {
        // Compare token with function and operator strings
        // check string for operator/function
        for (i = 0; pOprtDef[i]; i++)
        {
            std::size_t len(std::char_traits<char_type>::length(pOprtDef[i]));
            if (string_type(pOprtDef[i]) == string_type(szFormula + m_nPos, szFormula + m_nPos + len))
            {
                switch (i)
                {
                case  cmARG_SEP:
                    if (m_nSynFlags & noCOMMA)
                        throw ecUNEXPECTED_COMMA;

                    m_nSynFlags = noBC | noCBC | noOPT | noEND | noNEWLINE | noCOMMA | noPFX | noIC | noIO | noIF | noELSE;
                    a_Tok = ptr_tok_type(new GenericToken((ECmdCode)i, pOprtDef[i]));
                    break;

                case  cmELSE:
                    if (m_nSynFlags & noELSE)
                        throw ecMISPLACED_COLON;

                    m_nNumIfElse--;
                    if (m_nNumIfElse < 0)
                        throw ecMISPLACED_COLON;

                    m_nSynFlags = noBC | noCBC | noIO | noIC | noPFX | noEND | noNEWLINE | noCOMMA | noOPT | noIF | noELSE;
                    a_Tok = ptr_tok_type(new TokenIfThenElse(cmELSE));
                    break;

                case  cmIF:
                    if (m_nSynFlags & noIF)
                        throw ecUNEXPECTED_CONDITIONAL;

                    m_nNumIfElse++;
                    m_nSynFlags = noBC | noCBC | noIO | noPFX | noIC | noEND | noNEWLINE | noCOMMA | noOPT | noIF | noELSE;
                    a_Tok = ptr_tok_type(new TokenIfThenElse(cmIF));
                    break;

                case cmBO:
                    if (m_nSynFlags & noBO)
                        throw ecUNEXPECTED_PARENS;

                    if (m_eLastTokCode == cmFUNC)
                    {
                        m_nSynFlags = noOPT | noEND | noNEWLINE | noCOMMA | noPFX | noIC | noIO | noIF | noELSE | noCBC;
                    }
                    else
                    {
                        m_nSynFlags = noBC | noOPT | noEND | noNEWLINE | noCOMMA | noPFX | noIC | noIO | noIF | noELSE | noCBC;
                    }

                    m_nNumBra++;
                    a_Tok = ptr_tok_type(new GenericToken((ECmdCode)i, pOprtDef[i]));
                    break;

                case cmBC:
                    if (m_nSynFlags & noBC)
                        throw ecUNEXPECTED_PARENS;

                    m_nSynFlags = noBO | noVAR | noVAL | noFUN | noIFX | noCBO;
                    m_nNumBra--;

                    if (m_nNumBra < 0)
                        throw ecUNEXPECTED_PARENS;

                    a_Tok = ptr_tok_type(new GenericToken((ECmdCode)i, pOprtDef[i]));
                    break;

                case cmIO:
                    if (m_nSynFlags & noIO)
                        throw ecUNEXPECTED_SQR_BRACKET;

                    m_nSynFlags = noIC | noIO | noOPT | noPFX | noBC | noNEWLINE | noCBC | noCOMMA;
                    m_nNumIndex++;
                    a_Tok = ptr_tok_type(new GenericToken((ECmdCode)i, pOprtDef[i]));
                    break;

                case cmIC:
                    if (m_nSynFlags & noIC)
                        throw ecUNEXPECTED_SQR_BRACKET;

                    m_nSynFlags = noBO | noIFX | noCBO;
                    m_nNumIndex--;

                    if (m_nNumIndex < 0)
                        throw ecUNEXPECTED_SQR_BRACKET;

                    a_Tok = ptr_tok_type(new OprtIndex());
                    break;

                case cmCBO:
                    if (m_nSynFlags & noVAL)
                        throw ecUNEXPECTED_CURLY_BRACKET;

                    m_nSynFlags = noCBC | noIC | noIO | noOPT | noPFX | noBC | noNEWLINE | noCOMMA | noIF;
                    m_nNumCurly++;
                    a_Tok = ptr_tok_type(new GenericToken((ECmdCode)i, pOprtDef[i]));
                    break;

                case cmCBC:
                    if (m_nSynFlags & noIC)
                        throw ecUNEXPECTED_CURLY_BRACKET;

                    m_nSynFlags = noBO | noCBO | noIFX;
                    m_nNumCurly--;

                    if (m_nNumCurly < 0)
                        throw ecUNEXPECTED_CURLY_BRACKET;

                    a_Tok = ptr_tok_type(new OprtCreateArray());
                    break;

                default:  // The operator is listed in c_DefaultOprt, but not here. This is a bad thing...
                    throw ecINTERNAL_ERROR;
                } // switch operator id

                m_nPos += (int)len;
                return true;
            } // if operator string found
        } // end of for all operator strings
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Expr = m_sExpr;
        err.Ident = pOprtDef[i];
        err.Pos = m_nPos;
        throw ParserError(err);
    }

    return false;
}

//---------------------------------------------------------------------------
/** \brief Check for End of expression
*/
bool TokenReader::IsNewline(ptr_tok_type &a_Tok)
{
    // nicht nach:  bionop, infixop, argumentseparator, 
    // erlaubt nach:   Werten, variablen, schließenden klammern, schliessendem index
    bool bRet(false);
    try
    {
        if (m_sExpr[m_nPos] == '\n')
        {
            // Check if all brackets were closed
            if (m_nSynFlags & noNEWLINE)
                throw ecUNEXPECTED_NEWLINE;

            if (m_nNumBra > 0)
                throw ecMISSING_PARENS;

            if (m_nNumIndex > 0)
                throw ecMISSING_SQR_BRACKET;

            if (m_nNumCurly > 0)
                throw ecMISSING_CURLY_BRACKET;

            if (m_nNumIfElse > 0)
                throw(ecMISSING_ELSE_CLAUSE);

            m_nPos++;
            m_nSynFlags = sfSTART_OF_LINE;
            a_Tok = ptr_tok_type(new TokenNewline());
            bRet = true;
        }
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Ident = _T("");
        err.Expr = m_sExpr;
        err.Pos = m_nPos;
        throw ParserError(err);
    }

    return bRet;
}

//---------------------------------------------------------------------------
/** \brief Check for End of expression
*/
bool TokenReader::IsEOF(ptr_tok_type &a_Tok)
{
    bool bRet(false);
    try
    {
        if (m_sExpr.length() && m_nPos >= (int)m_sExpr.length())
        {
            if (m_nSynFlags & noEND)
                throw ecUNEXPECTED_EOF;

            if (m_nNumBra > 0)
                throw ecMISSING_PARENS;

            if (m_nNumCurly > 0)
                throw ecMISSING_CURLY_BRACKET;

            if (m_nNumIndex > 0)
                throw ecMISSING_SQR_BRACKET;

            if (m_nNumIfElse > 0)
                throw ecMISSING_ELSE_CLAUSE;

            m_nSynFlags = 0;
            a_Tok = ptr_tok_type(new GenericToken(cmEOE));
            bRet = true;
        }
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Ident = _T("");
        err.Expr = m_sExpr;
        err.Pos = m_nPos;
        throw ParserError(err);
    }

    return bRet;
}

//---------------------------------------------------------------------------
/** \brief Check if a string position contains a unary infix operator.
    \return true if a function token has been found false otherwise.
    */
bool TokenReader::IsInfixOpTok(ptr_tok_type &a_Tok)
{
    string_type sTok;
    int iEnd = ExtractToken(m_pParser->ValidInfixOprtChars(), sTok, m_nPos);

    if (iEnd == m_nPos)
        return false;

    try
    {
        // iteraterate over all infix operator strings
        oprt_ifx_maptype::const_iterator item = m_pInfixOprtDef->begin();
        for (item = m_pInfixOprtDef->begin(); item != m_pInfixOprtDef->end(); ++item)
        {
            if (sTok.find(item->first) != 0)
                continue;

            a_Tok = ptr_tok_type(item->second->Clone());
            m_nPos += (int)item->first.length();

            if (m_nSynFlags & noIFX)
                throw ecUNEXPECTED_OPERATOR;

            m_nSynFlags = noPFX | noIFX | noOPT | noBC | noIC | noIO | noEND | noCOMMA | noNEWLINE | noIF | noELSE;
            return true;
        }

        return false;
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Pos = m_nPos;
        err.Ident = a_Tok->GetIdent();
        err.Expr = m_sExpr;
        throw ParserError(err);
    }
}


//---------------------------------------------------------------------------
/** \brief Check expression for function tokens. */
bool TokenReader::IsFunTok(ptr_tok_type &a_Tok)
{
    if (m_pFunDef->size() == 0)
        return false;

    string_type sTok;
    int iEnd = ExtractToken(m_pParser->ValidNameChars(), sTok, m_nPos);
    if (iEnd == m_nPos)
        return false;

    try
    {
        fun_maptype::iterator item = m_pFunDef->find(sTok);
        if (item == m_pFunDef->end())
            return false;

        m_nPos = (int)iEnd;
        a_Tok = ptr_tok_type(item->second->Clone());
        a_Tok->Compile(_T("xxx"));

        if (m_nSynFlags & noFUN)
            throw ecUNEXPECTED_FUN;

        m_nSynFlags = sfALLOW_NONE ^ noBO;
        return true;
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Pos = m_nPos - (int)a_Tok->GetIdent().length();
        err.Ident = a_Tok->GetIdent();
        err.Expr = m_sExpr;
        throw ParserError(err);
    }
}

//---------------------------------------------------------------------------
/** \brief Check if a string position contains a unary post value operator. */
bool TokenReader::IsPostOpTok(ptr_tok_type &a_Tok)
{
    if (m_nSynFlags & noPFX)
    {
        // <ibg 2014-05-30/> Only look for postfix operators if they are allowed at the given position.
        //                   This will prevent conflicts with variable names such as: 
        //                   "sin(n)" where n is the postfix for "nano"
        return false;
        // </ibg>
    }

    // Tricky problem with equations like "3m+5":
    //     m is a postfix operator, + is a valid sign for postfix operators and 
    //     for binary operators parser detects "m+" as operator string and 
    //     finds no matching postfix operator.
    // 
    // This is a special case so this routine slightly differs from the other
    // token readers.

    // Test if there could be a postfix operator
    string_type sTok;
    int iEnd = ExtractToken(m_pParser->ValidOprtChars(), sTok, m_nPos);
    if (iEnd == m_nPos)
        return false;

    try
    {
        // iteraterate over all postfix operator strings
        oprt_pfx_maptype::const_iterator item;
        for (item = m_pPostOprtDef->begin(); item != m_pPostOprtDef->end(); ++item)
        {
            if (sTok.find(item->first) != 0)
                continue;

            a_Tok = ptr_tok_type(item->second->Clone());
            m_nPos += (int)item->first.length();

            if (m_nSynFlags & noPFX)
                throw ecUNEXPECTED_OPERATOR;

            m_nSynFlags = noVAL | noVAR | noFUN | noBO | noPFX /*| noIO*/ | noIF;
            return true;
        }

        return false;
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Pos = m_nPos - (int)a_Tok->GetIdent().length();
        err.Ident = a_Tok->GetIdent();
        err.Expr = m_sExpr;
        throw ParserError(err);
    }
}

//---------------------------------------------------------------------------
/** \brief Check if a string position contains a binary operator. */
bool TokenReader::IsOprt(ptr_tok_type &a_Tok)
{
    string_type sTok;
    int iEnd = ExtractToken(m_pParser->ValidOprtChars(), sTok, m_nPos);
    if (iEnd == m_nPos)
        return false;

    oprt_bin_maptype::reverse_iterator item;
    try
    {
        // Note:
        // All tokens in oprt_bin_maptype are have been sorted by their length
        // Long operators must come first! Otherwise short names (like: "add") that
        // are part of long token names (like: "add123") will be found instead 
        // of the long ones.
        // Length sorting is done with ascending length so we use a reverse iterator here.
        for (item = m_pOprtDef->rbegin(); item != m_pOprtDef->rend(); ++item)
        {
            if (sTok.find(item->first) != 0)
                continue;

            // operator found, check if we expect one...
            if (m_nSynFlags & noOPT)
            {
                // An operator was found but is not expected to occur at
                // this position of the formula, maybe it is an infix 
                // operator, not a binary operator. Both operator types
                // can use the same characters in their identifiers.
                if (IsInfixOpTok(a_Tok))
                    return true;

                // nope, it's no infix operator and we dont expect 
                // an operator
                throw ecUNEXPECTED_OPERATOR;
            }
            else
            {
                a_Tok = ptr_tok_type(item->second->Clone());

                m_nPos += (int)a_Tok->GetIdent().length();
                m_nSynFlags = noBC | noIO | noIC | noOPT | noCOMMA | noEND | noNEWLINE | noPFX | noIF | noELSE;
                return true;
            }
        }

        return false;
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Pos = m_nPos; // - (int)item->first.length();
        err.Ident = item->first;
        err.Expr = m_sExpr;
        throw ParserError(err);
    }
}

//---------------------------------------------------------------------------
/** \brief Check whether the token at a given position is a value token.

  Value tokens are either values or constants.

  \param a_Tok [out] If a value token is found it will be placed here.
  \return true if a value token has been found.
  */
bool TokenReader::IsValTok(ptr_tok_type &a_Tok)
{
    if (m_vValueReader.size() == 0)
        return false;

    stringstream_type stream(m_sExpr.c_str() + m_nPos);
    string_type sTok;

    try
    {
        // call the value recognition functions provided by the user
        // Call user defined value recognition functions
        int iSize = (int)m_vValueReader.size();
        Value val;
        for (int i = 0; i < iSize; ++i)
        {
            int iStart = m_nPos;
            if (m_vValueReader[i]->IsValue(m_sExpr.c_str(), m_nPos, val))
            {
                sTok.assign(m_sExpr.c_str(), iStart, m_nPos);
                if (m_nSynFlags & noVAL)
                    throw ecUNEXPECTED_VAL;

                m_nSynFlags = noVAL | noVAR | noFUN | noBO | noIFX | noIO;
                a_Tok = ptr_tok_type(val.Clone());
                a_Tok->SetIdent(string_type(sTok.begin(), sTok.begin() + (m_nPos - iStart)));
                return true;
            }
        }
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Pos = m_nPos;
        err.Ident = sTok;
        err.Expr = m_sExpr;
        err.Pos = m_nPos - (int)sTok.length();
        throw ParserError(err);
    }

    return false;
}

//---------------------------------------------------------------------------
/** \brief Check wheter a token at a given position is a variable token.
    \param a_Tok [out] If a variable token has been found it will be placed here.
    \return true if a variable token has been found.
    */
bool TokenReader::IsVarOrConstTok(ptr_tok_type &a_Tok)
{
    if (!m_pVarDef->size() && !m_pConstDef->size() && !m_pFunDef->size())
        return false;

    string_type sTok;
    int iEnd;
    try
    {
        iEnd = ExtractToken(m_pParser->ValidNameChars(), sTok, m_nPos);
        if (iEnd == m_nPos || (sTok.size() > 0 && sTok[0] >= _T('0') && sTok[0] <= _T('9')))
            return false;

        // Check for variables
        var_maptype::const_iterator item = m_pVarDef->find(sTok);
        if (item != m_pVarDef->end())
        {
            if (m_nSynFlags & noVAR)
                throw ecUNEXPECTED_VAR;

            m_nPos = iEnd;
            m_nSynFlags = noVAL | noVAR | noFUN | noBO | noIFX;
            a_Tok = ptr_tok_type(item->second->Clone());
            a_Tok->SetIdent(sTok);
            m_UsedVar[item->first] = item->second;  // Add variable to used-var-list
            return true;
        }

        // Check for constants
        item = m_pConstDef->find(sTok);
        if (item != m_pConstDef->end())
        {
            if (m_nSynFlags & noVAL)
                throw ecUNEXPECTED_VAL;

            m_nPos = iEnd;
            m_nSynFlags = noVAL | noVAR | noFUN | noBO | noIFX | noIO;
            a_Tok = ptr_tok_type(item->second->Clone());
            a_Tok->SetIdent(sTok);
            return true;
        }
    }
    catch (EErrorCodes e)
    {
        ErrorContext err;
        err.Errc = e;
        err.Pos = m_nPos;
        err.Ident = sTok;
        err.Expr = m_sExpr;
        throw ParserError(err);
    }

    return false;
}

//---------------------------------------------------------------------------
bool TokenReader::IsComment()
{
    return false;
}

//---------------------------------------------------------------------------
/** \brief Check wheter a token at a given position is an undefined variable.
    \param a_Tok [out] If a variable tom_pParser->m_vStringBufken has been found it will be placed here.
    \return true if a variable token has been found.
    \throw nothrow
    */
bool TokenReader::IsUndefVarTok(ptr_tok_type &a_Tok)
{
    string_type sTok;
    int iEnd = ExtractToken(m_pParser->ValidNameChars(), sTok, m_nPos);
    if (iEnd == m_nPos || (sTok.size() > 0 && sTok[0] >= _T('0') && sTok[0] <= _T('9')))
        return false;

    if (m_nSynFlags & noVAR)
    {
        ErrorContext err;
        err.Errc = ecUNEXPECTED_VAR;
        err.Ident = sTok;
        err.Expr = m_sExpr;
        err.Pos = m_nPos;
        throw ParserError(err);
    }

    // Create a variable token
    if (m_pParser->m_bAutoCreateVar)
    {
        ptr_val_type val(new Value);                   // Create new value token
        m_pDynVarShadowValues->push_back(val);         // push to the vector of shadow values 
        a_Tok = ptr_tok_type(new Variable(val.Get())); // bind variable to the new value item
        (*m_pVarDef)[sTok] = a_Tok;                    // add new variable to the variable list
    }
    else
        a_Tok = ptr_tok_type(new Variable(nullptr));      // bind variable to empty variable

    a_Tok->SetIdent(sTok);
    m_UsedVar[sTok] = a_Tok;     // add new variable to used-var-list

    m_nPos = iEnd;
    m_nSynFlags = noVAL | noVAR | noFUN | noBO | noIFX;
    return true;
}
} // namespace mu
