/** \file
    \brief Definition of functions for complex valued operations.

    <pre>
               __________                                 ____  ___
    _____  __ _\______   \_____ _______  ______ __________\   \/  /
    /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /
    |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \
    |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
          \/                     \/           \/     \/           \_/

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
    </pre>
    */
#include "mpFuncMatrix.h"

//--- Standard includes ----------------------------------------------------
#include <cmath>
#include <cassert>
#include <complex>
#include <iostream>

//--- Parser framework -----------------------------------------------------
#include "mpValue.h"
#include "mpError.h"


MUP_NAMESPACE_START

//-----------------------------------------------------------------------
//
//  class FunMatrixOnes
//
//-----------------------------------------------------------------------

FunMatrixOnes::FunMatrixOnes()
:ICallback(cmFUNC, _T("ones"), -1)
{}

//-----------------------------------------------------------------------
FunMatrixOnes::~FunMatrixOnes()
{}

//-----------------------------------------------------------------------
void FunMatrixOnes::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int argc)
{
    if (argc < 1 || argc>2)
    {
        ErrorContext err;
        err.Errc = ecINVALID_NUMBER_OF_PARAMETERS;
        err.Arg = argc;
        err.Ident = GetIdent();
        throw ParserError(err);
    }

    int m = a_pArg[0]->GetInteger(),
        n = (argc == 1) ? m : a_pArg[1]->GetInteger();

    if (m == n && n == 1)
    {
        *ret = 1.0; // unboxing of 1x1 matrices
    }
    else
    {
        *ret = matrix_type(m, n, 1.0);
    }
}

//-----------------------------------------------------------------------
const char_type* FunMatrixOnes::GetDesc() const
{
    return _T("ones(x [, y]) - Returns a matrix whose elements are all 1.");
}

//-----------------------------------------------------------------------
IToken* FunMatrixOnes::Clone() const
{
    return new FunMatrixOnes(*this);
}

//-----------------------------------------------------------------------
//
//  class FunMatrixZeros
//
//-----------------------------------------------------------------------

FunMatrixZeros::FunMatrixZeros()
    :ICallback(cmFUNC, _T("zeros"), -1)
{}

//-----------------------------------------------------------------------
FunMatrixZeros::~FunMatrixZeros()
{}

//-----------------------------------------------------------------------
void FunMatrixZeros::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int argc)
{
    if (argc < 1 || argc>2)
    {
        ErrorContext err;
        err.Errc = ecINVALID_NUMBER_OF_PARAMETERS;
        err.Arg = argc;
        err.Ident = GetIdent();
        throw ParserError(err);
    }

    int m = a_pArg[0]->GetInteger(),
        n = (argc == 1) ? m : a_pArg[1]->GetInteger();

    if (m == n && n == 1)
    {
        *ret = 0.0;  // unboxing of 1x1 matrices
    }
    else
    {
        *ret = matrix_type(m, n, 0.0);
    }
}

//-----------------------------------------------------------------------
const char_type* FunMatrixZeros::GetDesc() const
{
    return _T("zeros(x [, y]) - Returns a matrix whose elements are all 0.");
}

//-----------------------------------------------------------------------
IToken* FunMatrixZeros::Clone() const
{
    return new FunMatrixZeros(*this);
}

//-----------------------------------------------------------------------
//
//  class FunMatrixEye
//
//-----------------------------------------------------------------------

FunMatrixEye::FunMatrixEye()
    :ICallback(cmFUNC, _T("eye"), -1)
{}

//-----------------------------------------------------------------------
FunMatrixEye::~FunMatrixEye()
{}

//-----------------------------------------------------------------------
void FunMatrixEye::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int argc)
{
    if (argc < 1 || argc>2)
    {
        ErrorContext err;
        err.Errc = ecINVALID_NUMBER_OF_PARAMETERS;
        err.Arg = argc;
        err.Ident = GetIdent();
        throw ParserError(err);
    }

    int m = a_pArg[0]->GetInteger(),
        n = (argc == 1) ? m : a_pArg[1]->GetInteger();

    matrix_type eye(m, n, 0.0);

    for (int i = 0; i < std::min(m, n); ++i)
    {
        eye.At(i, i) = 1.0;
    }

    *ret = eye;
}

//-----------------------------------------------------------------------
const char_type* FunMatrixEye::GetDesc() const
{
    return _T("eye(x, y) - returns a matrix with ones on its diagonal and zeros elsewhere.");
}

//-----------------------------------------------------------------------
IToken* FunMatrixEye::Clone() const
{
    return new FunMatrixEye(*this);
}

//-----------------------------------------------------------------------
//
//  class FunMatrixSize
//
//-----------------------------------------------------------------------

FunMatrixSize::FunMatrixSize()
    :ICallback(cmFUNC, _T("size"), -1)
{}

//-----------------------------------------------------------------------
FunMatrixSize::~FunMatrixSize()
{}

//-----------------------------------------------------------------------
void FunMatrixSize::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int argc)
{
    if (argc != 1)
    {
        ErrorContext err;
        err.Errc = ecINVALID_NUMBER_OF_PARAMETERS;
        err.Arg = argc;
        err.Ident = GetIdent();
        throw ParserError(err);
    }

    matrix_type sz(1, 2, 0.0);
    sz.At(0, 0) = (float_type)a_pArg[0]->GetRows();
    sz.At(0, 1) = (float_type)a_pArg[0]->GetCols();
    *ret = sz;
}

//-----------------------------------------------------------------------
const char_type* FunMatrixSize::GetDesc() const
{
    return _T("size(x) - returns the matrix dimensions.");
}

//-----------------------------------------------------------------------
IToken* FunMatrixSize::Clone() const
{
    return new FunMatrixSize(*this);
}

MUP_NAMESPACE_END
