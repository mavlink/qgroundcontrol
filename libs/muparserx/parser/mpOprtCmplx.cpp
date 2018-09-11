/*
                           __________                                 ____  ___
                _____  __ _\______   \_____ _______  ______ __________\   \/  /
               /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /
               |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \
               |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
                     \/                     \/           \/     \/           \_/
               Copyright (C) 2016 Ingo Berg
               All rights reserved.

               muParserX - A C++ math parser library with array and string support
               Copyright (c) 2016, Ingo Berg
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
#include "mpOprtCmplx.h"
#include <iomanip>
#include <limits>

MUP_NAMESPACE_START

//-------------------------------------------------------------------------------------------------
//
//  class  OprtSignCmplx
//
//-------------------------------------------------------------------------------------------------

OprtSignCmplx::OprtSignCmplx()
:IOprtInfix(_T("-"), prINFIX)
{}

//-----------------------------------------------------------------------------------------------
void OprtSignCmplx::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc)
{
    MUP_VERIFY(a_iArgc == 1);

    if (a_pArg[0]->IsScalar())
    {
        float_type re = a_pArg[0]->GetFloat();
        float_type im = a_pArg[0]->GetImag();

        // Do not omit the test for zero! Multiplying 0 with -1 
        // will yield -0 on IEEE754 compliant implementations!
        // This would change the result of complex calculations:
        // 
        // i.e. sqrt(-1 + (-0)i) !=  sqrt(-1 + 0i)
        //                   -i  !=  i  
        cmplx_type v((re == 0) ? 0 : -re, (im == 0) ? 0 : -im);
        *ret = v;
    }
    else if (a_pArg[0]->GetType() == 'm')
    {
        Value v(a_pArg[0]->GetRows(), 0);
        for (int i = 0; i < a_pArg[0]->GetRows(); ++i)
        {
            v.At(i) = a_pArg[0]->At(i).GetComplex() * (float_type)-1.0;
        }
        *ret = v;
    }
    else
    {
        ErrorContext err;
        err.Errc = ecINVALID_TYPE;
        err.Type1 = a_pArg[0]->GetType();
        err.Type2 = 's';
        throw ParserError(err);
    }
}

//-----------------------------------------------------------------------------------------------
const char_type* OprtSignCmplx::GetDesc() const
{
    return _T("negative sign operator");
}

//-----------------------------------------------------------------------------------------------
IToken* OprtSignCmplx::Clone() const
{
    return new OprtSignCmplx(*this);
}

//-------------------------------------------------------------------------------------------------
//
// class OprtAddCmplx
//
//-------------------------------------------------------------------------------------------------

OprtAddCmplx::OprtAddCmplx()
:IOprtBin(_T("+"), (int)prADD_SUB, oaLEFT)
{}

//-----------------------------------------------------------------------------------------------
void OprtAddCmplx::Eval(ptr_val_type& ret, const ptr_val_type *a_pArg, int num)
{
    assert(num == 2);

    const IValue *arg1 = a_pArg[0].Get();
    const IValue *arg2 = a_pArg[1].Get();

    if (arg1->IsNonComplexScalar() && arg2->IsNonComplexScalar())
    {
        *ret = arg1->GetFloat() + arg2->GetFloat();
    }
    else if (arg1->GetType() == 'm' && arg2->GetType() == 'm')
    {
        // Matrix + Matrix
        *ret = arg1->GetArray() + arg2->GetArray();
    }
    else
    {
        if (!arg1->IsScalar())
            throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, GetExprPos(), GetIdent(), arg1->GetType(), 'c', 1));

        if (!arg2->IsScalar())
            throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, GetExprPos(), GetIdent(), arg2->GetType(), 'c', 2));

        *ret = cmplx_type(arg1->GetFloat() + arg2->GetFloat(),
            arg1->GetImag() + arg2->GetImag());
    }
}

//-----------------------------------------------------------------------------------------------
const char_type* OprtAddCmplx::GetDesc() const
{
    return _T("addition");
}

//-----------------------------------------------------------------------------------------------
IToken* OprtAddCmplx::Clone() const
{
    return new OprtAddCmplx(*this);
}

//-------------------------------------------------------------------------------------------------
//
// class OprtSubCmplx
//
//-------------------------------------------------------------------------------------------------

OprtSubCmplx::OprtSubCmplx()
:IOprtBin(_T("-"), (int)prADD_SUB, oaLEFT)
{}

//-----------------------------------------------------------------------------------------------
void OprtSubCmplx::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int num)
{
    assert(num == 2);

    const IValue *arg1 = a_pArg[0].Get();
    const IValue *arg2 = a_pArg[1].Get();
    if (a_pArg[0]->IsNonComplexScalar() && a_pArg[1]->IsNonComplexScalar())
    {
        *ret = arg1->GetFloat() - arg2->GetFloat();
    }
    else if (a_pArg[0]->GetType() == 'm' && a_pArg[1]->GetType() == 'm')
    {
        // Matrix + Matrix
        *ret = arg1->GetArray() - arg2->GetArray();
    }
    else
    {
        if (!a_pArg[0]->IsScalar())
            throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, GetExprPos(), GetIdent(), a_pArg[0]->GetType(), 'c', 1));

        if (!a_pArg[1]->IsScalar())
            throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, GetExprPos(), GetIdent(), a_pArg[1]->GetType(), 'c', 2));

        *ret = cmplx_type(a_pArg[0]->GetFloat() - a_pArg[1]->GetFloat(),
            a_pArg[0]->GetImag() - a_pArg[1]->GetImag());
    }
}

//-----------------------------------------------------------------------------------------------
const char_type* OprtSubCmplx::GetDesc() const
{
    return _T("subtraction");
}

//-----------------------------------------------------------------------------------------------
IToken* OprtSubCmplx::Clone() const
{
    return new OprtSubCmplx(*this);
}

//-------------------------------------------------------------------------------------------------
//
// class OprtMulCmplx
//
//-------------------------------------------------------------------------------------------------

OprtMulCmplx::OprtMulCmplx()
:IOprtBin(_T("*"), (int)prMUL_DIV, oaLEFT)
{}

//-----------------------------------------------------------------------------------------------
void OprtMulCmplx::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int num)
{
    assert(num == 2);
    IValue *arg1 = a_pArg[0].Get();
    IValue *arg2 = a_pArg[1].Get();
    *ret = (*arg1) * (*arg2);
}

//-----------------------------------------------------------------------------------------------
const char_type* OprtMulCmplx::GetDesc() const
{
    return _T("foo*bar - multiplication");
}

//-----------------------------------------------------------------------------------------------
IToken* OprtMulCmplx::Clone() const
{
    return new OprtMulCmplx(*this);
}

//-------------------------------------------------------------------------------------------------
//
// class OprtDivCmplx
//
//-------------------------------------------------------------------------------------------------


OprtDivCmplx::OprtDivCmplx()
:IOprtBin(_T("/"), (int)prMUL_DIV, oaLEFT)
{}

//-----------------------------------------------------------------------------------------------
/** \brief Implements the Division operator.
    \throw ParserError in case one of the arguments if
    nonnumeric or an array.

    */
void OprtDivCmplx::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int num)
{
    assert(num == 2);

    if (a_pArg[0]->IsNonComplexScalar() && a_pArg[1]->IsNonComplexScalar())
    {
        *ret = a_pArg[0]->GetFloat() / a_pArg[1]->GetFloat();
    }
    else
    {
        // multiplication of two imaginary numbers      
        float_type a = a_pArg[0]->GetFloat(),
            b = a_pArg[0]->GetImag(),
            c = a_pArg[1]->GetFloat(),
            d = a_pArg[1]->GetImag(),
            n = c*c + d*d;
        *ret = cmplx_type((a*c + b*d) / n, (b*c - a*d) / n);
    }
}

//-----------------------------------------------------------------------------------------------
const char_type* OprtDivCmplx::GetDesc() const
{
    return _T("division");
}

//-----------------------------------------------------------------------------------------------
IToken* OprtDivCmplx::Clone() const
{
    return new OprtDivCmplx(*this);
}

//-------------------------------------------------------------------------------------------------
//
// class OprtPowCmplx
//
//-------------------------------------------------------------------------------------------------

OprtPowCmplx::OprtPowCmplx()
:IOprtBin(_T("^"), (int)prPOW, oaRIGHT)
{}

//-----------------------------------------------------------------------------------------------
void OprtPowCmplx::Eval(ptr_val_type& ret, const ptr_val_type *arg, int argc)
{
    assert(argc == 2);

    if (arg[0]->IsComplex() || arg[1]->IsComplex() || (arg[0]->GetFloat() < 0 && !arg[1]->IsInteger()))
    {
        *ret = std::pow(arg[0]->GetComplex(), arg[1]->GetComplex());;
    }
    else
    {
        *ret = std::pow(arg[0]->GetFloat(), arg[1]->GetFloat());
    }
}

//-----------------------------------------------------------------------------------------------
const char_type* OprtPowCmplx::GetDesc() const
{
    return _T("raise x to the power of y");
}

//-----------------------------------------------------------------------------------------------
IToken* OprtPowCmplx::Clone() const
{
    return new OprtPowCmplx(*this);
}

} // namespace
