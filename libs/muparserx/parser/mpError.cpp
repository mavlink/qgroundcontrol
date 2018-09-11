/*
                           __________                                 ____  ___
                _____  __ _\______   \_____ _______  ______ __________\   \/  /
               /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /
               |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \
               |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
                     \/                     \/           \/     \/           \_/
               Copyright (C) 2016 Ingo Berg
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
               */
#include "mpError.h"
#include "mpIToken.h"
#include "mpParserMessageProvider.h"


MUP_NAMESPACE_START

std::unique_ptr<ParserMessageProviderBase> ParserErrorMsg::m_pInstance;

//------------------------------------------------------------------------------
const ParserMessageProviderBase& ParserErrorMsg::Instance()
{
    if (!m_pInstance.get())
    {
        m_pInstance.reset(new ParserMessageProviderEnglish);
        m_pInstance->Init();
    }

    return *m_pInstance;
}

//------------------------------------------------------------------------------
void ParserErrorMsg::Reset(ParserMessageProviderBase *pProvider)
{
    if (pProvider != nullptr)
    {
        m_pInstance.reset(pProvider);
        m_pInstance->Init();
    }
}

//------------------------------------------------------------------------------
string_type ParserErrorMsg::GetErrorMsg(EErrorCodes eError) const
{
    if (!m_pInstance.get())
        return string_type();
    else
        return m_pInstance->GetErrorMsg(eError);
}


//---------------------------------------------------------------------------
ParserErrorMsg::~ParserErrorMsg()
{}

//---------------------------------------------------------------------------
ParserErrorMsg::ParserErrorMsg()
{}

//---------------------------------------------------------------------------
//
//  Error context
//
//---------------------------------------------------------------------------

/** \brief Constructs an empty Error context structure. */
ErrorContext::ErrorContext(EErrorCodes a_iErrc,
    int a_iPos,
    string_type a_sIdent)
    :Expr()
    , Ident(a_sIdent)
    , Hint()
    , Errc(a_iErrc)
    , Type1(0)
    , Type2(0)
    , Arg(-1)
    , Pos(a_iPos)
{}

//---------------------------------------------------------------------------
ErrorContext::ErrorContext(EErrorCodes iErrc,
    int iPos,
    string_type sIdent,
    char_type cType1,
    char_type cType2,
    int nArg)
    :Expr()
    , Ident(sIdent)
    , Hint()
    , Errc(iErrc)
    , Type1(cType1)
    , Type2(cType2)
    , Arg(nArg)
    , Pos(iPos)
{}

//---------------------------------------------------------------------------
//
//  ParserError class
//
//---------------------------------------------------------------------------

ParserError::ParserError()
    :m_Err()
    , m_sMsg()
    , m_ErrMsg(ParserErrorMsg::Instance())
{}

//------------------------------------------------------------------------------
ParserError::ParserError(const string_type &sMsg)
    :m_Err()
    , m_sMsg(sMsg)
    , m_ErrMsg(ParserErrorMsg::Instance())
{}

//------------------------------------------------------------------------------
ParserError::ParserError(const ErrorContext &a_Err)
    :m_Err(a_Err)
    , m_sMsg()
    , m_ErrMsg(ParserErrorMsg::Instance())
{
    m_sMsg = m_ErrMsg.GetErrorMsg(a_Err.Errc);
}

//------------------------------------------------------------------------------
ParserError::ParserError(const ParserError &a_Obj)
    :m_Err(a_Obj.m_Err)
    , m_sMsg(a_Obj.m_sMsg)
    , m_ErrMsg(ParserErrorMsg::Instance())
{}

//------------------------------------------------------------------------------
ParserError& ParserError::operator=(const ParserError &a_Obj)
{
    if (this == &a_Obj)
        return *this;

    m_sMsg = a_Obj.m_sMsg;
    m_Err = a_Obj.m_Err;
    return *this;
}

//------------------------------------------------------------------------------
/** \brief Replace all occurences of a substring with another string. */
void ParserError::ReplaceSubString(string_type &sSource,
    const string_type &sFind,
    const string_type &sReplaceWith) const
{
    string_type sResult;
    string_type::size_type iPos(0), iNext(0);

    for (;;)
    {
        iNext = sSource.find(sFind, iPos);
        sResult.append(sSource, iPos, iNext - iPos);

        if (iNext == string_type::npos)
            break;

        sResult.append(sReplaceWith);
        iPos = iNext + sFind.length();
    }

    sSource.swap(sResult);
}


//------------------------------------------------------------------------------
/** \brief Replace all occurences of a substring with another string. */
void ParserError::ReplaceSubString(string_type &sSource,
    const string_type &sFind,
    int iReplaceWith) const
{
    stringstream_type stream;
    stream << iReplaceWith;
    ReplaceSubString(sSource, sFind, stream.str());
}

//------------------------------------------------------------------------------
/** \brief Replace all occurences of a substring with another string. */
void ParserError::ReplaceSubString(string_type &sSource,
    const string_type &sFind,
    char_type cReplaceWith) const
{
    stringstream_type stream;
    stream << cReplaceWith;
    ReplaceSubString(sSource, sFind, stream.str());
}

//------------------------------------------------------------------------------
void ParserError::Reset()
{
    m_sMsg = _T("");
    m_Err = ErrorContext();
}

//------------------------------------------------------------------------------
const string_type& ParserError::GetExpr() const
{
    return m_Err.Expr;
}

//------------------------------------------------------------------------------
string_type ParserError::GetMsg() const
{
    string_type sMsg(m_sMsg);
    ReplaceSubString(sMsg, _T("$EXPR$"), m_Err.Expr);
    ReplaceSubString(sMsg, _T("$IDENT$"), m_Err.Ident);
    ReplaceSubString(sMsg, _T("$POS$"), m_Err.Pos);
    ReplaceSubString(sMsg, _T("$ARG$"), m_Err.Arg);
    ReplaceSubString(sMsg, _T("$TYPE1$"), m_Err.Type1);
    ReplaceSubString(sMsg, _T("$TYPE2$"), m_Err.Type2);
    ReplaceSubString(sMsg, _T("$HINT$"), m_Err.Hint);
    return sMsg;
}

//------------------------------------------------------------------------------
ErrorContext& ParserError::GetContext()
{
    return m_Err;
}

//------------------------------------------------------------------------------
/** \brief Return the expression position related to the error.

  If the error is not related to a distinct position this will return -1
  */
int ParserError::GetPos() const
{
    return m_Err.Pos;
}

//------------------------------------------------------------------------------
/** \brief Return string related with this token (if available). */
const string_type& ParserError::GetToken() const
{
    return m_Err.Ident;
}

//------------------------------------------------------------------------------
/** \brief Return the error code. */
EErrorCodes ParserError::GetCode() const
{
    return m_Err.Errc;
}
}  // namespace mu
