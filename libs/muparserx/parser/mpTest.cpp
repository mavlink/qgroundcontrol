/** \file
    \brief Implementation of the unit test for muParserX.

    <pre>
                  __________                                 ____  ___
      _____  __  _\______   \_____ _______  ______ __________\   \/  /
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
    </pre>
    */
#include "mpTest.h"
#include "mpValue.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <complex>
#include <limits>

#define MUP_CONST_PI  3.141592653589793238462643
#define MUP_CONST_E   2.718281828459045235360287

using namespace std;


MUP_NAMESPACE_START

//-----------------------------------------------------------------------------------------------
//
// class OprtStrAdd
//
//-----------------------------------------------------------------------------------------------

class DbgSillyAdd : public IOprtBin
{
public:

    DbgSillyAdd()
        :IOprtBin(_T("++"), (int)prADD_SUB, oaLEFT)
    {}

    //-----------------------------------------------------------------------------------------------
    void Eval(ptr_val_type& ret, const ptr_val_type *arg, int argc)
    {
        assert(argc == 2);
        float_type a = arg[0]->GetFloat();
        float_type b = arg[1]->GetFloat();
        *ret = a + b;
    }

    //-----------------------------------------------------------------------------------------------
    const char_type* GetDesc() const
    {
        return _T("internally used operator without special meaning for unit testing");
    }

    //-----------------------------------------------------------------------------------------------
    IToken* Clone() const
    {
        return new DbgSillyAdd(*this);
    }
};

//------------------------------------------------------------------------------
class FunTest0 : public ICallback
{
public:
    FunTest0() : ICallback(cmFUNC, _T("test0"), 0)
    {}

    virtual void Eval(ptr_val_type &ret, const ptr_val_type * /*a_pArg*/, int /*a_iArgc*/)
    {
        *ret = 0.0;
    }

    virtual const char_type* GetDesc() const
    {
        return _T("");
    }

    virtual IToken* Clone() const
    {
        return new FunTest0(*this);
    }
}; // class FunTest0


//---------------------------------------------------------------------------
int ParserTester::c_iCount = 0;

//---------------------------------------------------------------------------
ParserTester::ParserTester()
:m_vTestFun()
, m_stream(&console())
{
    AddTest(&ParserTester::TestParserValue);
    AddTest(&ParserTester::TestUndefVar);
    AddTest(&ParserTester::TestErrorCodes);
    AddTest(&ParserTester::TestEqn);
    AddTest(&ParserTester::TestIfElse);
    AddTest(&ParserTester::TestStringFun);
    AddTest(&ParserTester::TestMatrix);
    AddTest(&ParserTester::TestComplex);
    AddTest(&ParserTester::TestVector);
    AddTest(&ParserTester::TestBinOp);
    AddTest(&ParserTester::TestPostfix);
    AddTest(&ParserTester::TestInfix);
    AddTest(&ParserTester::TestMultiArg);
    AddTest(&ParserTester::TestScript);
    AddTest(&ParserTester::TestValReader);
    AddTest(&ParserTester::TestIssueReports);


    ParserTester::c_iCount = 0;
}

//---------------------------------------------------------------------------
int ParserTester::TestIssueReports()
{
    int iNumErr = 0;
    *m_stream << _T("testing github issue reports...");


    // Github: Issue 55
    iNumErr += ThrowTest(_T("{0,{0}}*{0,{0}}*{,{0}}*{0,{0}0,{0}}*{0,{0}}*{,{0}}*{0}*{000}"), ecUNEXPECTED_COMMA);

    // Github: Issue 56
    iNumErr += ThrowTest(_T("0M[,1][0/1M[0M]M]"), ecUNEXPECTED_COMMA);

    // Github Issue 57:
    iNumErr += ThrowTest(_T("{?{{{{:44"), ecUNEXPECTED_CONDITIONAL);

    // Github Issue 60
    iNumErr += ThrowTest(_T("0<01?1=:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1<:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1>:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1-:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1-:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1-:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1-:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1-:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1+:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1*:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1/:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1&:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1<<:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("0<01?1>>:1"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("{ ? 0 : 7m}-{7, -00007m}-{7M}"), ecUNEXPECTED_CONDITIONAL);
    iNumErr += ThrowTest(_T("{ { { ? 2 }, 7:2 }*7m }"), ecUNEXPECTED_CONDITIONAL);

    // Not too happy about the undefined code, but better than a crash of an assertion at runtime
    iNumErr += ThrowTest(_T("{0<0?0,0:0<0}"), ecUNDEFINED);

    // Github Issue 63
    iNumErr += ThrowTest(_T("0<0-0--eye()"), ecINVALID_NUMBER_OF_PARAMETERS);

    Assessment(iNumErr);
    return iNumErr;

}

//---------------------------------------------------------------------------
int ParserTester::TestUndefVar()
{
    int iNumErr = 0;
    *m_stream << _T("testing implicit definition of undefined variables...");

    // Test 1: No variables defined, test detection of undefined variables
    {
        ParserX p;
        p.SetExpr(_T("a+b+c+d"));
        const mup::var_maptype &expr_var = p.GetExprVar();
        const mup::var_maptype &var = p.GetVar();

        // The expression contains 4 undefined variables
        if (expr_var.size() != 4)
            iNumErr++;

        // There are no variables defined
        if (var.size() != 0)
            iNumErr++;
    }

    // Test 2: Variables were defined explicitely, test detection of variables
    {
        ParserX p;

        // Now define the variables and perform the check again
        Value vVarVal[] = { 1.0, 2.0, 3.0, 4.0 };
        p.DefineVar(_T("a"), Variable(&vVarVal[0]));
        p.DefineVar(_T("b"), Variable(&vVarVal[1]));
        p.DefineVar(_T("c"), Variable(&vVarVal[2]));
        p.DefineVar(_T("d"), Variable(&vVarVal[3]));

        p.SetExpr(_T("a+b+c+d"));
        const mup::var_maptype &expr_var = p.GetExprVar();
        const mup::var_maptype &var = p.GetVar();

        // The expression contains 4 undefined variables
        if (expr_var.size() != 4)
            iNumErr++;

        // There are no variables defined
        if (var.size() != 4)
            iNumErr++;
    }

    // Test 3: Variables were defined implicitels, test detection of undefined variables
    {
        ParserX p;

        // Now define the variables and perform the check again
        p.EnableAutoCreateVar(true);

        p.SetExpr(_T("a+b+c+d"));
        const mup::var_maptype &expr_var = p.GetExprVar();
        const mup::var_maptype &var = p.GetVar();

        // The expression contains 4 undefined variables
        if (expr_var.size() != 4)
            iNumErr++;

        // There are no variables defined
        if (var.size() != 4)
            iNumErr++;
    }

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestMatrix()
{
    int iNumErr = 0;
    *m_stream << _T("testing matrix calculations...");

    Value unity(3, 3, 0);
    unity.At(0, 0) = 1.0;
    unity.At(1, 1) = 1.0;
    unity.At(2, 2) = 1.0;

    Value va(3, 0);
    va.At(0) = 1.0;
    va.At(1) = 2.0;
    va.At(2) = 3.0;

    //Value m2(3, 3, 0);
    //m2.At(0, 0) = 1;  m2.At(0, 1) = 2;  m2.At(0, 2) = 3;
    //m2.At(1, 0) = 4;  m2.At(1, 1) = 5;  m2.At(1, 2) = 6;
    //m2.At(2, 0) = 7;  m2.At(2, 1) = 8;  m2.At(2, 2) = 9;

    Value m1_plus_m2(3, 3, 0);
    m1_plus_m2.At(0, 0) = 2.0;  m1_plus_m2.At(0, 1) = 2.0;  m1_plus_m2.At(0, 2) = 3.0;
    m1_plus_m2.At(1, 0) = 4.0;  m1_plus_m2.At(1, 1) = 6.0;  m1_plus_m2.At(1, 2) = 6.0;
    m1_plus_m2.At(2, 0) = 7.0;  m1_plus_m2.At(2, 1) = 8.0;  m1_plus_m2.At(2, 2) = 10.0;

    Value m2_minus_m1(3, 3, 0);
    m2_minus_m1.At(0, 0) = 0.0;  m2_minus_m1.At(0, 1) = 2.0;  m2_minus_m1.At(0, 2) = 3.0;
    m2_minus_m1.At(1, 0) = 4.0;  m2_minus_m1.At(1, 1) = 4.0;  m2_minus_m1.At(1, 2) = 6.0;
    m2_minus_m1.At(2, 0) = 7.0;  m2_minus_m1.At(2, 1) = 8.0;  m2_minus_m1.At(2, 2) = 8.0;

    Value m2_times_10(3, 3, 0);
    m2_times_10.At(0, 0) = 10.0;  m2_times_10.At(0, 1) = 20.0;  m2_times_10.At(0, 2) = 30.0;
    m2_times_10.At(1, 0) = 40.0;  m2_times_10.At(1, 1) = 50.0;  m2_times_10.At(1, 2) = 60.0;
    m2_times_10.At(2, 0) = 70.0;  m2_times_10.At(2, 1) = 80.0;  m2_times_10.At(2, 2) = 90.0;

    Value va_times_vb_transp(3, 3, 0);
    va_times_vb_transp.At(0, 0) = 4.0;   va_times_vb_transp.At(0, 1) = 3.0;   va_times_vb_transp.At(0, 2) = 2.0;
    va_times_vb_transp.At(1, 0) = 8.0;   va_times_vb_transp.At(1, 1) = 6.0;   va_times_vb_transp.At(1, 2) = 4.0;
    va_times_vb_transp.At(2, 0) = 12.0;  va_times_vb_transp.At(2, 1) = 9.0;   va_times_vb_transp.At(2, 2) = 6.0;

    Value size_3x6(1, 2, 0);
    size_3x6.At(0, 0) = 3.0;
    size_3x6.At(0, 1) = 6.0;

    Value size_3x3(1, 2, 0);
    size_3x3.At(0, 0) = 3.0;
    size_3x3.At(0, 1) = 3.0;

    Value size_3x1(1, 2, 0);
    size_3x1.At(0, 0) = 3.0;
    size_3x1.At(0, 1) = 1.0;

    Value size_1x3(1, 2, 0);
    size_1x3.At(0, 0) = 1.0;
    size_1x3.At(0, 1) = 3.0;


    // Check matrix dimension mismatch error
    iNumErr += ThrowTest(_T("\"hallo\"+m1"), ecEVAL);
    iNumErr += ThrowTest(_T("m1+\"hallo\""), ecEVAL);
    iNumErr += ThrowTest(_T("va+m1"), ecMATRIX_DIMENSION_MISMATCH);
    iNumErr += ThrowTest(_T("m1+va"), ecMATRIX_DIMENSION_MISMATCH);
    iNumErr += ThrowTest(_T("va-m1"), ecMATRIX_DIMENSION_MISMATCH);
    iNumErr += ThrowTest(_T("m1-va"), ecMATRIX_DIMENSION_MISMATCH);
    iNumErr += ThrowTest(_T("va*m1"), ecMATRIX_DIMENSION_MISMATCH);
    iNumErr += ThrowTest(_T("va+eye(2)"), ecMATRIX_DIMENSION_MISMATCH);

    // Issue 63:
    iNumErr += ThrowTest(_T("0-0-eye()"), ecINVALID_NUMBER_OF_PARAMETERS);

    iNumErr += ThrowTest(_T("m1[1]"), ecINDEX_DIMENSION);
    iNumErr += ThrowTest(_T("m1[1,2,3]"), ecINDEX_DIMENSION);
    iNumErr += ThrowTest(_T("va[1,2]"), ecINDEX_OUT_OF_BOUNDS); // va has 1 column, 3 rows -> the coulumn index is referencing the third column
    iNumErr += ThrowTest(_T("a+m1"), ecEVAL);
    iNumErr += ThrowTest(_T("m1+a"), ecEVAL);
    iNumErr += ThrowTest(_T("a-m1"), ecEVAL);
    iNumErr += ThrowTest(_T("m1-a"), ecEVAL);
    iNumErr += ThrowTest(_T("va[,1]"), ecUNEXPECTED_COMMA);
    iNumErr += ThrowTest(_T("va[{1]"), ecMISSING_CURLY_BRACKET);
    iNumErr += ThrowTest(_T("{,1}"), ecUNEXPECTED_COMMA);

    // sample expressions
    iNumErr += EqnTest(_T("m1"), unity, true);
    iNumErr += EqnTest(_T("m1*m1"), unity, true);
    iNumErr += EqnTest(_T("m1+m2"), m1_plus_m2, true);
    iNumErr += EqnTest(_T("m2-m1"), m2_minus_m1, true);
    iNumErr += EqnTest(_T("10*m2"), m2_times_10, true);
    iNumErr += EqnTest(_T("m2*10"), m2_times_10, true);
    iNumErr += EqnTest(_T("5*m2*b"), m2_times_10, true);
    iNumErr += EqnTest(_T("b*m2*5"), m2_times_10, true);
    iNumErr += EqnTest(_T("m1*va"), va, true);

    // ones
    Value ones_3(3, 1.0);
    Value ones_3x3(3, 3, 1.0);
    iNumErr += ThrowTest(_T("ones(1,2,3)"), ecINVALID_NUMBER_OF_PARAMETERS);
    iNumErr += ThrowTest(_T("ones()"), ecINVALID_NUMBER_OF_PARAMETERS);
    iNumErr += EqnTest(_T("ones(1,1)"), 1.0, true);
    iNumErr += EqnTest(_T("ones(1)"), 1.0, true);
    iNumErr += EqnTest(_T("ones(3,3)"), ones_3x3, true);
    iNumErr += EqnTest(_T("ones(3,1)"), ones_3, true);
    iNumErr += EqnTest(_T("ones(3)"), ones_3, true);

    iNumErr += EqnTest(_T("size(ones(3,3))"), size_3x3, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(ones(1,3))"), size_1x3, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(ones(3,1))"), size_3x1, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(ones(3))"), size_3x3, true);  // check return value dimension

    // zeros
    iNumErr += ThrowTest(_T("zeros()"), ecINVALID_NUMBER_OF_PARAMETERS);
    iNumErr += EqnTest(_T("size(zeros(3,3))"), size_3x3, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(zeros(1,3))"), size_1x3, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(zeros(3,1))"), size_3x1, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(zeros(3))"), size_3x3, true);  // check return value dimension

    // eye
    iNumErr += ThrowTest(_T("eye()"), ecINVALID_NUMBER_OF_PARAMETERS);
    iNumErr += EqnTest(_T("size(eye(3,3))"), size_3x3, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(eye(1,3))"), size_1x3, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(eye(3,1))"), size_3x1, true);  // check return value dimension
    iNumErr += EqnTest(_T("size(eye(3))"), size_3x3, true);  // check return value dimension

    iNumErr += EqnTest(_T("size(eye(3,6))"), size_3x6, true);  // check return value dimension

    // transposition
    iNumErr += EqnTest(_T("va'*vb"), 16.0, true);
    iNumErr += EqnTest(_T("2*va'*vb"), 32.0, true);
    iNumErr += EqnTest(_T("va*vb'"), va_times_vb_transp, true);

    // index operator
    // erster index: Zeilenindex, zweiter index: Spaltenindex
    iNumErr += EqnTest(_T("va[0]"), 1.0, true);
    iNumErr += EqnTest(_T("va[1]"), 2.0, true);
    iNumErr += EqnTest(_T("va[2]"), 3.0, true);
    // Use two dimensional index operator on a vector
    iNumErr += EqnTest(_T("va[0,0]"), 1.0, true);
    iNumErr += EqnTest(_T("va[1,0]"), 2.0, true);
    iNumErr += EqnTest(_T("va[2,0]"), 3.0, true);

    // Now test the same with a transposed vector:
    iNumErr += EqnTest(_T("va'[0]"), 1.0, true);
    iNumErr += EqnTest(_T("va'[1]"), 2.0, true);
    iNumErr += EqnTest(_T("va'[2]"), 3.0, true);
    // Use two dimensional index operator on a vector
    iNumErr += EqnTest(_T("va'[0,0]"), 1.0, true);
    iNumErr += EqnTest(_T("va'[0,1]"), 2.0, true);
    iNumErr += EqnTest(_T("va'[0,2]"), 3.0, true);
    iNumErr += EqnTest(_T("(va')[0,2]"), 3.0, true); // <- Index operator after closing bracket is ok

    // vector creation
    iNumErr += EqnTest(_T("{1,2,3}'"), va, true);
    iNumErr += EqnTest(_T("{a,2,3}'"), va, true);		// that was an actual bug: variable a was overwritten

    // assignment to element:
    iNumErr += ThrowTest(_T("va'[0]=123"), ecASSIGNEMENT_TO_VALUE);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestComplex()
{
    int iNumErr = 0;
    *m_stream << _T("testing complex calculations...");

    // complex numbers
    // ca=1+i, cb=2+3i, cc=3+4i
    iNumErr += EqnTest(_T("ca==1+i"), true, true);
    iNumErr += EqnTest(_T("ca==ca"), true, true);
    iNumErr += EqnTest(_T("ca!=1+i"), false, true);
    iNumErr += EqnTest(_T("ca!=ca"), false, true);
    iNumErr += EqnTest(_T("ca!=cb"), true, true);
    iNumErr += EqnTest(_T("ca!=va"), true, true);
    iNumErr += EqnTest(_T("ca==va"), false, true);

    // When comparing complex number Matlab/Octave compare only the real part
    // I'll do the same...
    iNumErr += EqnTest(_T("ca<10+i"), true, true);
    iNumErr += EqnTest(_T("ca>10+i"), false, true);
    iNumErr += EqnTest(_T("ca<=10+i"), true, true);
    iNumErr += EqnTest(_T("ca>=10+i"), false, true);
    iNumErr += EqnTest(_T("ca<=1"), true, true);
    iNumErr += EqnTest(_T("ca>=1"), true, true);

    // complex numbers
    iNumErr += EqnTest(_T("i*i"), -1.0, true, 0);
    iNumErr += EqnTest(_T("1i"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("norm(3+4i)"), 25.0, true, 0);
    iNumErr += EqnTest(_T("norm(4i+3)"), 25.0, true, 0);
    iNumErr += EqnTest(_T("norm(3i+4)"), 25.0, true, 0);
    iNumErr += EqnTest(_T("real(4.1i+3.1)"), (float_type)3.1, true, 0);
    iNumErr += EqnTest(_T("imag(3.1i+4.1)"), (float_type)3.1, true, 0);
    iNumErr += EqnTest(_T("real(3.1)"), (float_type)3.1, true, 0);
    iNumErr += EqnTest(_T("imag(2.1i)"), (float_type)2.1, true, 0);
    iNumErr += EqnTest(_T("-(4i+5)"), cmplx_type(-5, -4), true, 0);
    iNumErr += EqnTest(_T("sqrt(-1)"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("(-1)^0.5"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("(-3)^(4/3)"), std::pow(cmplx_type(-3, 0),
        cmplx_type(4.0 / 3, 0)), true, 0);

    // Issue 41:  Complex pow of small numbers zeros out the imaginary part
    //            https://code.google.com/p/muparserx/issues/detail?id=41
    iNumErr += EqnTest(_T("(1e-15 + 1e-15*i) ^ 2"), std::pow(cmplx_type(1e-15, 1e-15), 2), true, 0);


    iNumErr += EqnTest(_T("sqrt(i*i)"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("sqrt(f)"), cmplx_type(0, 1), true, 1);
    iNumErr += EqnTest(_T("sqrt(2-3)"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("sqrt(a-b)"), cmplx_type(0, 1), true, 2);
    iNumErr += EqnTest(_T("sqrt((2-3))"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("sqrt((a-b))"), cmplx_type(0, 1), true, 2);
    iNumErr += EqnTest(_T("sqrt(-(1))"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("sqrt((-1))"), cmplx_type(0, 1), true, 0);
    iNumErr += EqnTest(_T("sqrt(-(-1))"), cmplx_type(1, 0), true, 0);
    iNumErr += EqnTest(_T("sqrt(1)"), cmplx_type(1, 0), true, 0);
    iNumErr += EqnTest(_T("a=1+2i"), cmplx_type(1, 2), true, 1);
    iNumErr += EqnTest(_T("-(1+2i)"), cmplx_type(-1, -2), true, 0);
    iNumErr += EqnTest(_T("-(-1-2i)"), cmplx_type(1, 2), true, 0);
    iNumErr += EqnTest(_T("a*i"), cmplx_type(0, 1), true, 1);
    iNumErr += EqnTest(_T("-(a+b*i)"), cmplx_type(-1, -2), true, 2);
    iNumErr += EqnTest(_T("-(-a-b*i)"), cmplx_type(1, 2), true, 2);
    iNumErr += EqnTest(_T("(2+4i)*(8-6i)"), cmplx_type(40, 20), true, 0);

    // Issue 17:  Wrong result on complex power.
    iNumErr += EqnTest(_T("(-0.27 + 0.66*i)^2"), cmplx_type(-0.3627, -0.3564), true, 0);
    iNumErr += EqnTest(_T("(-1+5i)^2"), cmplx_type(-24, -10), true, 0);

    iNumErr += EqnTest(_T("c=(a=1+2i)"), cmplx_type(1, 2), true, 2);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestParserValue()
{
    int iNumErr = 0;
    *m_stream << _T("testing parser value types...");

    // Define values and variables for each type
    Value bVal = true;
    Value fVal = (float_type)3.14;
    Value sVal = string_type(_T("hello world"));
    Value sVal1 = _T("hello world");   // Test assignment from const char* to string
    Value cVal = cmplx_type(1, 1);
    Value aVal(2, 0);
    aVal.At(0) = (float_type)2.0;
    aVal.At(1) = (float_type)3.0;

    // Create a 3x3 matrix
    Value matrix(3, 0);
    matrix.At(0) = Value(3, 0);
    matrix.At(1) = Value(3, 0);
    matrix.At(2) = Value(3, 0);

    Variable bVar(&bVal),
             fVar(&fVal),
             sVar(&sVal),
             sVar1(&sVal1),
             cVar(&cVal),
             aVar(&aVal);

    // Check the value types
    try
    {
        // Test if matrix values do work
        if (!matrix.IsMatrix() || matrix.GetRows() != 3)
            iNumErr++;

        std::size_t sz = matrix.GetRows();
        for (std::size_t i = 0; i < sz; ++i)
        {
            std::size_t dim_row = matrix.At(i).GetRows();
            if (dim_row != 3)
            {
                iNumErr++;
                console() << _T("\n  Array dimension mismatch in matrix row ") << i
                    << _T(" (expected=3; dim=") << dim_row << _T(")");
            }
        }


        // test type checking of values
        if (!fVal.IsScalar() || fVal.IsMatrix()  || fVal.GetType()  != 'f') iNumErr++;
        if (!cVal.IsScalar() || cVal.IsMatrix()  || cVal.GetType()  != 'c') iNumErr++;
        if (aVal.IsScalar()  || !aVal.IsMatrix() || aVal.GetType()  != 'm') iNumErr++;
        if (sVal.IsScalar()  || sVal.IsMatrix()  || sVal.GetType()  != 's') iNumErr++;
        if (sVal1.IsScalar() || sVal1.IsMatrix() || sVal1.GetType() != 's') iNumErr++;
        if (bVal.IsScalar()  || bVal.IsMatrix()  || bVal.GetType()  != 'b') iNumErr++;

        // test type checking of variables
        if (!fVar.IsScalar() || fVar.IsMatrix()  || fVar.GetType() != 'f')  iNumErr++;
        if (!cVar.IsScalar() || cVar.IsMatrix()  || cVar.GetType() != 'c')  iNumErr++;
        if (aVar.IsScalar()  || !aVar.IsMatrix() || aVar.GetType() != 'm')  iNumErr++;
        if (sVar.IsScalar()  || sVar.IsMatrix()  || sVar.GetType() != 's')  iNumErr++;
        if (sVar1.IsScalar() || sVar1.IsMatrix() || sVar1.GetType() != 's') iNumErr++;
        if (bVar.IsScalar()  || bVar.IsMatrix()  || bVar.GetType() != 'b')  iNumErr++;

        // Issue 33: https://code.google.com/p/muparserx/issues/detail?id=33
        // Remark: Type information was not properly set when invoking +=, -= operators
        {
            Value x = 1.0;
            Value y = cmplx_type(0, 1);
            x += y;
            if (x.GetImag() != 1 || x.GetFloat() != 1 || x.GetType() != 'c')
            {
                *m_stream << _T("\nValue::operator+=(...) failed.");
                iNumErr++;
            }

            x = 1.0;
            y = cmplx_type(0, 1);
            x -= y;
            if (x.GetImag() != -1 || x.GetFloat() != 1 || x.GetType() != 'c')
            {
                *m_stream << _T("\nValue::operator-=(...) failed.");
                iNumErr++;
            }
        }
    }
    catch (...)
    {
        iNumErr++;
    }

    bool bError;

#define VALUE_THROWCHECK(VAR, FAIL, MEMBER) \
    bError = (FAIL);                        \
    try                                     \
    {                                       \
    VAR.MEMBER();                         \
    }                                       \
    catch (...)                              \
    {                                       \
    bError ^= true;                      \
    }                                       \
    iNumErr += (bError) ? 1 : 0;            \
    c_iCount++;

    // Check if the getter functions really throw an exception 
    // when used with an incorrect value type
    // Case 1:  test float values
    VALUE_THROWCHECK(fVal, false, GetFloat)
        VALUE_THROWCHECK(fVal, false, GetImag)
        VALUE_THROWCHECK(fVal, true, GetBool)
        VALUE_THROWCHECK(fVal, true, GetString)
        VALUE_THROWCHECK(fVal, true, GetArray)
        // for variables
        VALUE_THROWCHECK(fVar, false, GetFloat)
        VALUE_THROWCHECK(fVar, false, GetImag)
        VALUE_THROWCHECK(fVar, true, GetBool)
        VALUE_THROWCHECK(fVar, true, GetString)
        VALUE_THROWCHECK(fVar, true, GetArray)

        // Case 2:  test bool values
        VALUE_THROWCHECK(bVal, false, GetFloat)
        VALUE_THROWCHECK(bVal, true, GetImag)
        VALUE_THROWCHECK(bVal, false, GetBool)
        VALUE_THROWCHECK(bVal, true, GetString)
        VALUE_THROWCHECK(bVal, true, GetArray)
        // for variables
        VALUE_THROWCHECK(bVar, false, GetFloat)
        VALUE_THROWCHECK(bVar, true, GetImag)
        VALUE_THROWCHECK(bVar, false, GetBool)
        VALUE_THROWCHECK(bVar, true, GetString)
        VALUE_THROWCHECK(bVar, true, GetArray)

        // Case 3:  test string values
        //    VALUE_THROWCHECK(sVal, true,  GetFloat)
        VALUE_THROWCHECK(sVal, true, GetImag)
        VALUE_THROWCHECK(sVal, true, GetBool)
        VALUE_THROWCHECK(sVal, false, GetString)
        VALUE_THROWCHECK(sVal, true, GetArray)
        // for variables
        //    VALUE_THROWCHECK(sVar, true,  GetFloat)
        VALUE_THROWCHECK(sVar, true, GetImag)
        VALUE_THROWCHECK(sVar, true, GetBool)
        VALUE_THROWCHECK(sVar, false, GetString)
        VALUE_THROWCHECK(sVar, true, GetArray)

        // Case 4:  test array values
        //    VALUE_THROWCHECK(aVal, true,  GetFloat)
        VALUE_THROWCHECK(aVal, true, GetImag)
        VALUE_THROWCHECK(aVal, true, GetBool)
        VALUE_THROWCHECK(aVal, true, GetString)
        VALUE_THROWCHECK(aVal, false, GetArray)
        // for variables
        //    VALUE_THROWCHECK(aVar, true,  GetFloat)
        VALUE_THROWCHECK(aVar, true, GetImag)
        VALUE_THROWCHECK(aVar, true, GetBool)
        VALUE_THROWCHECK(aVar, true, GetString)
        VALUE_THROWCHECK(aVar, false, GetArray)

        // Case 5:  test complex values
        //    VALUE_THROWCHECK(cVal, false,  GetFloat)
        VALUE_THROWCHECK(cVal, false, GetImag)
        VALUE_THROWCHECK(cVal, true, GetBool)
        VALUE_THROWCHECK(cVal, true, GetString)
        VALUE_THROWCHECK(cVal, true, GetArray)
        // for variables
        //    VALUE_THROWCHECK(cVar, false,  GetFloat)
        VALUE_THROWCHECK(cVar, false, GetImag)
        VALUE_THROWCHECK(cVar, true, GetBool)
        VALUE_THROWCHECK(cVar, true, GetString)
        VALUE_THROWCHECK(cVar, true, GetArray)
#undef VALUE_THROWCHECK

        Assessment(iNumErr);
    return iNumErr;
}


//---------------------------------------------------------------------------
int ParserTester::TestErrorCodes()
{
    int  iNumErr = 0;
    *m_stream << _T("testing error codes...");

    iNumErr += ThrowTest(_T("a,b"), ecUNEXPECTED_COMMA);
    iNumErr += ThrowTest(_T("(a,b)"), ecUNEXPECTED_COMMA);
    iNumErr += ThrowTest(_T("((a,b))"), ecUNEXPECTED_COMMA);
    iNumErr += ThrowTest(_T("2*1,2"), ecUNEXPECTED_COMMA);
    iNumErr += ThrowTest(_T("sin(1,2)"), ecTOO_MANY_PARAMS);

    // Invalid expression
    // Issue 20: http://code.google.com/p/muparserx/issues/detail?id=20
    iNumErr += ThrowTest(_T(" "), ecUNEXPECTED_EOF);

    iNumErr += ThrowTest(_T("sin(nonexistent_var)"), ecUNASSIGNABLE_TOKEN, 4, _T("nonexistent_var"));

    // Invalid function argument types
    iNumErr += ThrowTest(_T("sin(\"test\")"), ecEVAL, 0);
    iNumErr += ThrowTest(_T("max(1, \"test\")"), ecEVAL, 0);
    iNumErr += ThrowTest(_T("max(1,sin(8), \"t\")"), ecEVAL, 0);
    iNumErr += ThrowTest(_T("str2dbl(sin(3.14))"), ecEVAL, 0);

    // Invalid unary operator argument types
    iNumErr += ThrowTest(_T("\"test\"n"), ecEVAL, 6); // (nano can only be applied to floats)
    iNumErr += ThrowTest(_T("(1+3i)/(8*9i)+\"hallo\""), ecEVAL);
    iNumErr += ThrowTest(_T("(1+3i)/(8*9i)-\"hallo\""), ecEVAL);
    iNumErr += ThrowTest(_T("(1+3i)/(8*9i)*\"hallo\""), ecEVAL);
    iNumErr += ThrowTest(_T("(1+3i)/(8*9i)/\"hallo\""), ecEVAL);
    iNumErr += ThrowTest(_T("10+va"), ecEVAL, 2);

    // Type conflicts in binary operators
    iNumErr += ThrowTest(_T("\"test\" // 8"), ecEVAL, 7);
    iNumErr += ThrowTest(_T("8//\"test\""), ecEVAL, 1);
    iNumErr += ThrowTest(_T("5//8"), ecEVAL, 1);
    iNumErr += ThrowTest(_T("\"t\"//sin(8)"), ecEVAL, 3);
    iNumErr += ThrowTest(_T("sin(8)//\"t\""), ecEVAL, 6);

    // Unexpected end of expression
    iNumErr += ThrowTest(_T("3+"), ecUNEXPECTED_EOF);
    iNumErr += ThrowTest(_T("8*"), ecUNEXPECTED_EOF);
    iNumErr += ThrowTest(_T("3+("), ecUNEXPECTED_EOF);
    iNumErr += ThrowTest(_T("3+sin"), ecUNEXPECTED_EOF);
    iNumErr += ThrowTest(_T("(2+"), ecUNEXPECTED_EOF);

    iNumErr += ThrowTest(_T("3+)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("3)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("(3))"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("()"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("(2+)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("sin(cos)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("sin(())"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("sin()"), ecTOO_FEW_PARAMS);
    iNumErr += ThrowTest(_T("sin)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("pi)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("a)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("2(-m)"), ecUNEXPECTED_PARENS);
    iNumErr += ThrowTest(_T("2(m)"), ecUNEXPECTED_PARENS);

    iNumErr += ThrowTest(_T("(1+2"), ecMISSING_PARENS);
    iNumErr += ThrowTest(_T("((3)"), ecMISSING_PARENS);

    // The behaviour in the next function depends on whether
    // implicit variable creation is allowed or not. (momentarily its active)
    iNumErr += ThrowTest(_T("5z)"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("sin(3)xyz"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("5t6"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("5 t 6"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("ksdfj"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("-m"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("m4"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("sin(m)"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("m m"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("m(8)"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("4 + m"), ecUNASSIGNABLE_TOKEN);

    // unexpected operator
    iNumErr += ThrowTest(_T("5+*3)"), ecUNEXPECTED_OPERATOR);

    // unexpected comma (used without a function)
    iNumErr += ThrowTest(_T(",3"), ecUNEXPECTED_COMMA);
    iNumErr += ThrowTest(_T("sin(,sin(8))"), ecUNEXPECTED_COMMA);

    // unexpected variable
    iNumErr += ThrowTest(_T("a _xxx_ b"), ecUNASSIGNABLE_TOKEN, 2);   // if a variable factory is installed ecUNEXPECTED_VAR

    iNumErr += ThrowTest(_T("sin(3)cos(3)"), ecUNEXPECTED_FUN);
    iNumErr += ThrowTest(_T("sin(3)3"), ecUNEXPECTED_VAL);
    iNumErr += ThrowTest(_T("sin(3)+"), ecUNEXPECTED_EOF);

    // value recognition
    iNumErr += ThrowTest(_T("0x"), ecUNASSIGNABLE_TOKEN);  // incomplete hex value
    iNumErr += ThrowTest(_T("1+0x"), ecUNASSIGNABLE_TOKEN);  // incomplete hex value  
    iNumErr += ThrowTest(_T("a+0x"), ecUNASSIGNABLE_TOKEN);  // incomplete hex value  

    // index operator
    iNumErr += ThrowTest(_T("3n[1]"), ecINDEX_OUT_OF_BOUNDS);  // Indexing a scalar is ok, but this index is out of bounds (0 would be ok)
    iNumErr += ThrowTest(_T("min(3,]"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("sin(]"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("va[]"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("3+]"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("sin[a)"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("1+[8]"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("1[8]"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("[1]"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("]1"), ecUNEXPECTED_SQR_BRACKET);
    iNumErr += ThrowTest(_T("va[[3]]"), ecUNEXPECTED_SQR_BRACKET);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestStringFun()
{
    int  iNumErr = 0;
    *m_stream << _T("testing string functions...");

    // escape sequences
    iNumErr += EqnTest(_T("\"\\\"quoted_string\\\"\""), _T("\"quoted_string\""), true); // "\"quoted_string\"" -> "quoted_string"
    iNumErr += EqnTest(_T("\"\\\"\\\"\""), _T("\"\""), true);                           // "\"\""              -> ""
    iNumErr += EqnTest(_T("\"\\\\\""), _T("\\"), true);                                 // "\\"                -> \     (single backslash)

    // String functions
    iNumErr += EqnTest(_T("strlen(\"12345\")"), 5.0, true);
    iNumErr += EqnTest(_T("strlen(toupper(\"abcde\"))"), 5.0, true);
    iNumErr += EqnTest(_T("sin(0)+(float)strlen(\"12345\")"), (float_type)5.0, true);
    iNumErr += EqnTest(_T("10*(float)strlen(toupper(\"12345\"))"), (float_type)50.0, true);
    iNumErr += EqnTest(_T("\"hello \"//\"world\""), string_type(_T("hello world")), true);
    iNumErr += EqnTest(_T("toupper(\"hello \")//\"world\""), string_type(_T("HELLO world")), true);
    iNumErr += EqnTest(_T("\"hello \"//toupper(\"world\")//\" !!!\""), string_type(_T("hello WORLD !!!")), true);

    Assessment(iNumErr);

    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestPostfix()
{
    int iNumErr = 0;
    *m_stream << _T("testing postfix operators...");

    // application
    iNumErr += EqnTest(_T("1n"), (float_type)1e-9, true);
    iNumErr += EqnTest(_T("8n"), (float_type)8e-9, true);
    iNumErr += EqnTest(_T("8n"), (float_type)123.0, false);
    iNumErr += EqnTest(_T("3m+5"), (float_type)5.003, true);
    iNumErr += EqnTest(_T("1000m"), (float_type)1.0, true);
    iNumErr += EqnTest(_T("1000 m"), (float_type)1.0, true);
    iNumErr += EqnTest(_T("(a)m"), (float_type)1e-3, true);
    iNumErr += EqnTest(_T("-(a)m"), (float_type)-1e-3, true);
    iNumErr += EqnTest(_T("-2m"), (float_type)-2e-3, true);
    iNumErr += EqnTest(_T("a++b"), 3.0, true);
    iNumErr += EqnTest(_T("a ++ b"), 3.0, true);
    iNumErr += EqnTest(_T("1++2"), 3.0, true);
    iNumErr += EqnTest(_T("1 ++ 2"), 3.0, true);
    iNumErr += EqnTest(_T("2+(a*1000)m"), (float_type)3.0, true);
    // some incorrect results
    iNumErr += EqnTest(_T("1000m"), (float_type)0.1, false);
    iNumErr += EqnTest(_T("(a)m"), (float_type)2.0, false);
    // factorial operator
    iNumErr += EqnTest(_T("5!"), 120.0, true);
    iNumErr += EqnTest(_T("-5!"), -120.0, true);
    iNumErr += ThrowTest(_T("(-5)!"), ecDOMAIN_ERROR);

    // Special tests for systems not supporting IEEE 754
    if (!std::numeric_limits<float_type>::is_iec559)
    {
        iNumErr += ThrowTest(_T("123456!"), ecOVERFLOW);
    }

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestInfix()
{
    int iNumErr = 0;
    *m_stream << _T("testing infix operators...");

    float_type a = 1;
    float_type b = 2;

    iNumErr += EqnTest(_T("-1"), (float_type)-1.0, true);
    iNumErr += EqnTest(_T("-(-1)"), (float_type)1.0, true);
    iNumErr += EqnTest(_T("-(-1)*2"), (float_type)2.0, true);
    iNumErr += EqnTest(_T("-(-2)*sqrt(4)"), (float_type)4.0, true);
    iNumErr += EqnTest(_T("-a"), -a, true);
    iNumErr += EqnTest(_T("-(a)"), -(a), true);
    iNumErr += EqnTest(_T("-(-a)"), -(-a), true);
    iNumErr += EqnTest(_T("-(-a)*2"), -(-a) * 2, true);
    iNumErr += EqnTest(_T("-(8)"), (float_type)-8.0, true);
    iNumErr += EqnTest(_T("-8"), (float_type)-8.0, true);
    iNumErr += EqnTest(_T("-(2+1)"), (float_type)-3.0, true);
    //iNumErr += EqnTest("-(f1of1(1+2*3)+1*2)", -9.0, true);
    //iNumErr += EqnTest("-(-f1of1(1+2*3)+1*2)", 5.0, true);
    iNumErr += EqnTest(_T("-sin(8)"), (float_type)-0.989358, true);
    iNumErr += EqnTest(_T("-sin(8)"), (float_type)0.989358, false);
    iNumErr += EqnTest(_T("3-(-a)"), (float_type)4.0, true);
    iNumErr += EqnTest(_T("3--a"), (float_type)4.0, true);
    iNumErr += EqnTest(_T("2++4"), (float_type)6.0, true);
    iNumErr += EqnTest(_T("--1"), (float_type)1.0, true);

    // sign precedence
    // Issue 14: https://code.google.com/p/muparserx/issues/detail?id=14
    iNumErr += EqnTest(_T("-3^2"), -9.0, true);
    iNumErr += EqnTest(_T("-b^2^3-b^8"), -std::pow(b, std::pow(2.0, 3.0)) - std::pow(b, 8), true);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestMultiArg()
{
    int iNumErr = 0;
    *m_stream << _T("testing multiarg functions...");

    // Multiarg functions being called without any parameters
    iNumErr += ThrowTest(_T("min()"), ecTOO_FEW_PARAMS);
    iNumErr += ThrowTest(_T("max()"), ecTOO_FEW_PARAMS);
    iNumErr += ThrowTest(_T("sum()"), ecTOO_FEW_PARAMS);

    // application
    iNumErr += EqnTest(_T("max(1,8,9,(int)6)"), (float_type)9.0, true);
    iNumErr += EqnTest(_T("max((int)6, 1+2, 4, -9)"), (float_type)6.0, true);
    iNumErr += EqnTest(_T("min((int)6, 1+2, 4, -9)"), (float_type)-9.0, true);

    // 
    iNumErr += EqnTest(_T("a=test0()"), (float_type)0, true);
    iNumErr += EqnTest(_T("b=a+test0()"), (float_type)1, true);

    // added as response to this bugreport:
    // http://code.google.com/p/muparserx/issues/detail?id=1
    // cause of the error: Function tokens were not cloned in the tokenreader when beeing found.
    //                     a pointer to the one and only function onject was returned instead
    //                     consequently the argument counter was overwritten by the second function call 
    //                     causing an assertion later on.
    iNumErr += EqnTest(_T("sum(1,2)/sum(3,4)"), (float_type)0.428571, true);
    iNumErr += EqnTest(_T("3/sum(3,4,5)"), (float_type)0.25, true);
    iNumErr += EqnTest(_T("sum(3)/sum(3,4,5)"), (float_type)0.25, true);
    iNumErr += EqnTest(_T("sum(3)+sum(3,4,5)"), (float_type)15, true);
    iNumErr += EqnTest(_T("sum(1,2)/sum(3,4,5)"), (float_type)0.25, true);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestVector()
{
    int  iNumErr = 0;
    *m_stream << _T("testing vector operations...");

    // Vector operations
    iNumErr += ThrowTest(_T("10+2*va"), ecEVAL);   // fail: number + vector
    iNumErr += ThrowTest(_T("10+va*2"), ecEVAL);   // fail: number + vector
    iNumErr += ThrowTest(_T("va+vc"), ecMATRIX_DIMENSION_MISMATCH);   // fail: vectors of different size
    iNumErr += ThrowTest(_T("va-vc"), ecMATRIX_DIMENSION_MISMATCH);   // fail: vectors of different size
    iNumErr += ThrowTest(_T("va*vc"), ecMATRIX_DIMENSION_MISMATCH);   // fail: vectors of different size
    iNumErr += ThrowTest(_T("va*vb"), ecMATRIX_DIMENSION_MISMATCH);   // fail: matrix dimension mismatch
    iNumErr += ThrowTest(_T("va*va"), ecMATRIX_DIMENSION_MISMATCH);   // fail: matrix dimension mismatch
    iNumErr += ThrowTest(_T("(va*vb)*b"), ecMATRIX_DIMENSION_MISMATCH);   // fail: matrix dimension mismatch
    iNumErr += ThrowTest(_T("va[1.23]"), ecTYPE_CONFLICT_IDX, 7);   // fail: float value used as index
    iNumErr += ThrowTest(_T("va[sin(8)]"), ecTYPE_CONFLICT_IDX, 9);   // fail: float value used as index
    iNumErr += ThrowTest(_T("va[-1]"), ecINDEX_OUT_OF_BOUNDS); // fail: negative value used as an index
    iNumErr += ThrowTest(_T("va[c]"), ecINDEX_OUT_OF_BOUNDS);
    iNumErr += ThrowTest(_T("va[(3)]"), ecINDEX_OUT_OF_BOUNDS);
    iNumErr += ThrowTest(_T("a[1]"), ecINDEX_OUT_OF_BOUNDS); // indexing a scalar is ok, but this index is out of bounds (0 would be ok...)
    iNumErr += ThrowTest(_T("va[1"), ecMISSING_SQR_BRACKET);
    iNumErr += ThrowTest(_T("va[1]]"), ecUNEXPECTED_SQR_BRACKET);

    //iNumErr += ThrowTest(_T("va==9"),      ecEVAL);
    //iNumErr += ThrowTest(_T("va==a"),      ecEVAL);
    //iNumErr += ThrowTest(_T("a==va"),      ecEVAL);
    //iNumErr += ThrowTest(_T("9==va"),      ecEVAL);

    Value v(3, 0);
    v.At(0) = (float_type)5.0;
    v.At(1) = (float_type)5.0;
    v.At(2) = (float_type)5.0;
    iNumErr += EqnTest(_T("va+vb"), v, true);

    v.At(0) = (float_type)5.0;
    v.At(1) = (float_type)5.0;
    v.At(2) = (float_type)6.0;
    iNumErr += EqnTest(_T("va+vb"), v, false);

    v.At(0) = (float_type)-1.0;
    v.At(1) = (float_type)-2.0;
    v.At(2) = (float_type)-3.0;
    iNumErr += EqnTest(_T("-va"), v, true);

    iNumErr += EqnTest(_T("sizeof(va+vb)"), 3.0, true);
    iNumErr += EqnTest(_T("sizeof(va-vb)"), 3.0, true);

    iNumErr += EqnTest(_T("va==vb"), false, true);
    iNumErr += EqnTest(_T("va!=vb"), true, true);
    //iNumErr += EqnTest(_T("va<vb"),  false, true);
    //iNumErr += EqnTest(_T("va>vb"),  true, true);
    //iNumErr += EqnTest(_T("va<=vb"), false, true);
    //iNumErr += EqnTest(_T("va>=vb"), true, true);

    iNumErr += EqnTest(_T("vb[va[0]]"), 3.0, true);
    iNumErr += EqnTest(_T("m1[0,0]+m1[1,1]+m1[2,2]"), 3.0, true);
    iNumErr += EqnTest(_T("vb[m1[0,0]]"), 3.0, true);

    iNumErr += EqnTest(_T("m1[0,0]=2"), 2.0, true);
    iNumErr += EqnTest(_T("m1[1,1]=2"), 2.0, true);
    iNumErr += EqnTest(_T("m1[2,2]=2"), 2.0, true);
    iNumErr += EqnTest(_T("va[0]=12.3"), (float_type)12.3, true);
    iNumErr += EqnTest(_T("va[1]=12.3"), (float_type)12.3, true);
    iNumErr += EqnTest(_T("va[2]=12.3"), (float_type)12.3, true);

    iNumErr += EqnTest(_T("va[0]"), 1.0, true);
    iNumErr += EqnTest(_T("va[1]"), 2.0, true);
    iNumErr += EqnTest(_T("va[2]"), 3.0, true);
    iNumErr += EqnTest(_T("(va[2])"), 3.0, true);
    iNumErr += EqnTest(_T("va[a]"), 2.0, true);
    iNumErr += EqnTest(_T("(va[a])"), 2.0, true);
    iNumErr += EqnTest(_T("va[b]"), 3.0, true);
    iNumErr += EqnTest(_T("va[(2)]"), 3.0, true);
    iNumErr += EqnTest(_T("va[-(-2)]"), 3.0, true);
    iNumErr += EqnTest(_T("(va[(2)])"), 3.0, true);
    iNumErr += EqnTest(_T("(va[-(-2)])"), 3.0, true);
    iNumErr += EqnTest(_T("va[1+1]"), 3.0, true);
    iNumErr += EqnTest(_T("va[(int)sin(8)+1]"), 2.0, true);

    iNumErr += EqnTest(_T("va[2]+4"), 7.0, true);
    iNumErr += EqnTest(_T("4+va[2]"), 7.0, true);
    iNumErr += EqnTest(_T("va[2]*4"), 12.0, true);
    iNumErr += EqnTest(_T("4*va[2]"), 12.0, true);
    iNumErr += EqnTest(_T("va[2]+a"), 4.0, true);
    iNumErr += EqnTest(_T("a+va[2]"), 4.0, true);
    iNumErr += EqnTest(_T("va[2]*b"), 6.0, true);
    iNumErr += EqnTest(_T("b*va[2]"), 6.0, true);

    // Issue 68 (and related issues):
    iNumErr += EqnTest(_T("(abs(-3)+2)>=min(6,5)"), true, true);
    iNumErr += EqnTest(_T("(abs(-3))>abs(2)"), true, true);
    iNumErr += EqnTest(_T("min(1,2,-3)>-4"), true, true);
    iNumErr += EqnTest(_T("(abs(-3))>-2"), true, true);
    iNumErr += EqnTest(_T("abs(-3)>abs(2)"), true, true);

    // Issue 42:
    // https://code.google.com/p/muparserx/issues/detail?id=42
    v.At(0) = (float_type)1.0;
    v.At(1) = (float_type)0.0;
    v.At(2) = (float_type)0.0;
    iNumErr += EqnTest(_T("{1,0,0}'"), v, true);
    iNumErr += EqnTest(_T("{(1),0,0}'"), v, true);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestBinOp()
{
    int  iNumErr = 0;
    *m_stream << _T("testing binary operators...");
    float_type a = 1;

    // standard aperators
    iNumErr += EqnTest(_T("1+7"), (float_type)8.0, true);
    iNumErr += EqnTest(_T("10-1"), (float_type)9.0, true);
    iNumErr += EqnTest(_T("3*4"), (float_type)12.0, true);
    iNumErr += EqnTest(_T("10/2"), (float_type)5.0, true);
    // operator associativity
    iNumErr += EqnTest(_T("2^2^3"), (float_type)256.0, true);
    iNumErr += EqnTest(_T("3+4*2/(1-5)^2^3"), (float_type)3.0001220703125, true);
    iNumErr += EqnTest(_T("1/2/3"), (float_type)1.0 / (float_type)6.0, true);

    // operator precedencs
    iNumErr += EqnTest(_T("1+2-3*4/5^6"), (float_type)2.99923, true);
    iNumErr += EqnTest(_T("a+b-c*4/5^6"), (float_type)2.99923, true);
    iNumErr += EqnTest(_T("1^2/3*4-5+6"), (float_type)2.3333, true);
    iNumErr += EqnTest(_T("a^b/c*4-5+6"), (float_type)2.3333, true);
    iNumErr += EqnTest(_T("1+2*3"), (float_type)7.0, true);
    iNumErr += EqnTest(_T("a+b*c"), (float_type)7.0, true);
    iNumErr += EqnTest(_T("(1+2)*3"), (float_type)9.0, true);
    iNumErr += EqnTest(_T("(a+b)*c"), (float_type)9.0, true);
    iNumErr += EqnTest(_T("(1+2)*(-3)"), (float_type)-9.0, true);
    iNumErr += EqnTest(_T("(a+b)*(-c)"), (float_type)-9.0, true);
    iNumErr += EqnTest(_T("2/4"), (float_type)0.5, true);
    iNumErr += EqnTest(_T("4&4"), 4.0, true);
    iNumErr += EqnTest(_T("2+2&(a+b+c)"), 4.0, true);
    iNumErr += EqnTest(_T("3&3"), 3.0, true);
    iNumErr += EqnTest(_T("c&3"), 3.0, true);
    iNumErr += EqnTest(_T("(c)&3"), 3.0, true);
    iNumErr += EqnTest(_T("(a+b)&3"), 3.0, true);
    iNumErr += EqnTest(_T("(a+b+c)&6"), 6.0, true);
    iNumErr += EqnTest(_T("(1+2+3)&6"), 6.0, true);
    iNumErr += EqnTest(_T("3&c"), 3.0, true);
    iNumErr += EqnTest(_T("(a<<1)+2"), 4.0, true);
    iNumErr += EqnTest(_T("(a<<2)+2"), 6.0, true);
    iNumErr += EqnTest(_T("(a<<3)+2"), 10.0, true);
    iNumErr += EqnTest(_T("(a<<4)+2"), 18.0, true);
    iNumErr += EqnTest(_T("(a<<5)+2"), 34.0, true);
    iNumErr += EqnTest(_T("1<<31"), (float_type)2147483648, true);
    iNumErr += EqnTest(_T("-1<<31"), (float_type)-2147483648.0, true);
    iNumErr += EqnTest(_T("1<<45"), (float_type)35184372088832.0, true);
    iNumErr += EqnTest(_T("-1<<45"), (float_type)-35184372088832.0, true);
    iNumErr += EqnTest(_T("8<<-2"), 2.0, true);
    iNumErr += EqnTest(_T("8<<-4"), 0.0, true);
    // Issue 25: http://code.google.com/p/muparserx/issues/detail?id=25
    iNumErr += ThrowTest(_T("55<<2222222"), ecOVERFLOW);
    // Issue 16: http://code.google.com/p/muparserx/issues/detail?id=16
    iNumErr += EqnTest(_T("true  == true && false"), true == true && false, true);
    iNumErr += EqnTest(_T("false == true && false"), false == true && false, true);
    iNumErr += EqnTest(_T("a==1.0 && a==1.0"), a == 1.0 && a == 1.0, true);

    // bool operators for comparing values
    iNumErr += EqnTest(_T("a<b"), true, true);
    iNumErr += EqnTest(_T("b>a"), true, true);
    iNumErr += EqnTest(_T("a>a"), false, true);
    iNumErr += EqnTest(_T("a<a"), false, true);
    iNumErr += EqnTest(_T("a>a"), false, true);
    iNumErr += EqnTest(_T("a<=a"), true, true);
    iNumErr += EqnTest(_T("a<=b"), true, true);
    iNumErr += EqnTest(_T("b<=a"), false, true);
    iNumErr += EqnTest(_T("a>=a"), true, true);
    iNumErr += EqnTest(_T("b>=a"), true, true);
    iNumErr += EqnTest(_T("a>=b"), false, true);
    // The following equations were raising type conflict errors once
    // since the result of sqrt(1) is 1 which is an integer as fas as muParserX
    // is concerned:
    iNumErr += EqnTest(_T("sqrt(a)<sin(8)"), false, true);
    iNumErr += EqnTest(_T("sqrt(a)<=sin(8)"), false, true);
    iNumErr += EqnTest(_T("sqrt(a)>sin(8)"), true, true);
    iNumErr += EqnTest(_T("sqrt(a)>=sin(8)"), true, true);
    iNumErr += EqnTest(_T("sqrt(a)==sin(8)"), false, true);
    iNumErr += EqnTest(_T("sqrt(a)!=sin(8)"), true, true);
    iNumErr += EqnTest(_T("sqrt(a)+1.01"), (float_type)2.01, true);
    iNumErr += EqnTest(_T("sqrt(a)-1.01"), (float_type)-0.01, true);

    // interaction with sign operator
    iNumErr += EqnTest(_T("3-(-a)"), 4.0, true);
    iNumErr += EqnTest(_T("3--a"), 4.0, true);

    // Problems with small bogus real/imag values introduced due to limited floating point accuracy
    iNumErr += EqnTest(_T("(-2)^3"), -8.0, true);                   // may introduce incorrect imaginary value (When computed with the log/exp formula: -8 + 2.93e-15i)
    iNumErr += EqnTest(_T("imag((-2)^3)==0"), true, true);        // may introduce incorrect imaginary value (When computed with the log/exp formula: -8 + 2.93e-15i)

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestIfElse()
{
    int  iNumErr = 0;
    *m_stream << _T("testing if-else conditional...");

    float_type a = 1;

    // Test error detection
    iNumErr += ThrowTest(_T(": 2"), ecMISPLACED_COLON);
    iNumErr += ThrowTest(_T("? 1 : 2"), ecUNEXPECTED_CONDITIONAL);
    iNumErr += ThrowTest(_T("(a<b) ? (b<c) ? 1 : 2"), ecMISSING_ELSE_CLAUSE);
    iNumErr += ThrowTest(_T("(a<b) ? 1"), ecMISSING_ELSE_CLAUSE);
    iNumErr += ThrowTest(_T("(a<b) ? a"), ecMISSING_ELSE_CLAUSE);
    iNumErr += ThrowTest(_T("(a<b) ? a+b"), ecMISSING_ELSE_CLAUSE);
    iNumErr += ThrowTest(_T("a : b"), ecMISPLACED_COLON, 2);
    iNumErr += ThrowTest(_T("1 : 2"), ecMISPLACED_COLON, 2);
    iNumErr += ThrowTest(_T("(true) ? 1 : 2 : 3"), ecMISPLACED_COLON);

    iNumErr += ThrowTest(_T("1==?"), ecUNEXPECTED_CONDITIONAL);
    iNumErr += ThrowTest(_T("1+?"), ecUNEXPECTED_CONDITIONAL);  // bin oprt + ?
    iNumErr += ThrowTest(_T("1m?"), ecUNEXPECTED_CONDITIONAL);  // postfix + ?
    iNumErr += ThrowTest(_T("-?"), ecUNEXPECTED_CONDITIONAL);  // infix + ?

    iNumErr += EqnTest(_T("(true) ? 128 : 255"), 128.0, true);
    iNumErr += EqnTest(_T("(1<2) ? 128 : 255"), 128.0, true);
    iNumErr += EqnTest(_T("(a<b) ? 128 : 255"), 128.0, true);
    iNumErr += EqnTest(_T("((a>b) ? true : false) ? 1 : 2"), 2.0, true);
    iNumErr += EqnTest(_T("((a>b) ? true : false) ? 1 : sum((a>b) ? 1 : 2)"), 2.0, true);
    iNumErr += EqnTest(_T("((a>b) ? false : true) ? 1 : sum((a>b) ? 1 : 2)"), 1.0, true);
    iNumErr += EqnTest(_T("(true) ? 10 : 11"), 10.0, true);
    iNumErr += EqnTest(_T("(true) ? a+b : c+d"), 3.0, true);
    iNumErr += EqnTest(_T("(true) ? false : true"), false, true);
    iNumErr += EqnTest(_T("(false) ? 10 : 11"), 11.0, true);
    iNumErr += EqnTest(_T("(false) ? a+b : c+d"), 1.0, true);
    iNumErr += EqnTest(_T("(false) ? false : true"), true, true);
    iNumErr += EqnTest(_T("(a<b) ? 10 : 11"), 10.0, true);
    iNumErr += EqnTest(_T("(a>b) ? 10 : 11"), 11.0, true);
    iNumErr += EqnTest(_T("(a<b) ? c : d"), 3.0, true);
    iNumErr += EqnTest(_T("(a>b) ? c : d"), -2.0, true);
    iNumErr += EqnTest(_T("(a>b) ? true : false"), false, true);

    // With multiarg functions
    iNumErr += EqnTest(_T("sum((a>b) ? 1 : 2)"), 2.0, true);
    iNumErr += EqnTest(_T("sum((a>b) ? 1 : 2, 100)"), 102.0, true);
    iNumErr += EqnTest(_T("sum((true) ? 1 : 2)"), 1.0, true);
    iNumErr += EqnTest(_T("sum((true) ? 1 : 2, 100)"), 101.0, true);
    iNumErr += EqnTest(_T("sum(3, (a>b) ? 3 : 10)"), 13.0, true);
    iNumErr += EqnTest(_T("sum(3, (a<b) ? 3 : 10)"), 6., true);
    iNumErr += EqnTest(_T("sum(3, (a>b) ? 3 : 10)*10"), 130.0, true);
    iNumErr += EqnTest(_T("sum(3, (a<b) ? 3 : 10)*10"), 60.0, true);
    iNumErr += EqnTest(_T("10*sum(3, (a>b) ? 3 : 10)"), 130.0, true);
    iNumErr += EqnTest(_T("10*sum(3, (a<b) ? 3 : 10)"), 60.0, true);
    iNumErr += EqnTest(_T("(a<b) ? sum(3, (a<b) ? 3 : 10)*10 : 99"), 60.0, true);
    iNumErr += EqnTest(_T("(a>b) ? sum(3, (a<b) ? 3 : 10)*10 : 99"), 99.0, true);
    iNumErr += EqnTest(_T("(a<b) ? sum(3, (a<b) ? 3 : 10,10,20)*10 : 99"), 360.0, true);
    iNumErr += EqnTest(_T("(a>b) ? sum(3, (a<b) ? 3 : 10,10,20)*10 : 99"), 99.0, true);
    iNumErr += EqnTest(_T("(a>b) ? sum(3, (a<b) ? 3 : 10,10,20)*10 : sum(3, (a<b) ? 3 : 10)*10"), 60.0, true);

    iNumErr += EqnTest(_T("(a<b)&&(a<b) ? 128 : 255"), 128.0, true);
    iNumErr += EqnTest(_T("(a>b)&&(a<b) ? 128 : 255"), 255.0, true);
    iNumErr += EqnTest(_T("(1<2)&&(1<2) ? 128 : 255"), 128.0, true);
    iNumErr += EqnTest(_T("(1>2)&&(1<2) ? 128 : 255"), 255.0, true);
    iNumErr += EqnTest(_T("((1<2)&&(1<2)) ? 128 : 255"), 128.0, true);
    iNumErr += EqnTest(_T("((1>2)&&(1<2)) ? 128 : 255"), 255.0, true);
    iNumErr += EqnTest(_T("((a<b)&&(a<b)) ? 128 : 255"), 128.0, true);
    iNumErr += EqnTest(_T("((a>b)&&(a<b)) ? 128 : 255"), 255.0, true);

    // nested conditionals with brackets
    iNumErr += EqnTest(_T("(a<b) ? ((b<c) ? 2*(a+b) : 2) : 3"), 6., true);
    iNumErr += EqnTest(_T("(a<b) ? 3 : ((b<c) ? 2*(a+b) : 2)"), 3., true);
    iNumErr += EqnTest(_T("(a<b) ? ((b>c) ? 1 : 2) : 3"), 2., true);
    iNumErr += EqnTest(_T("(a>b) ? ((b<c) ? 1 : 2) : 3"), 3., true);
    iNumErr += EqnTest(_T("(a>b) ? ((b>c) ? 1 : 2) : 3"), 3., true);

    // nested conditionals without brackets
    iNumErr += EqnTest(_T("(a<b) ? (b<c) ? 1 : 2 : 3"), 1., true);
    iNumErr += EqnTest(_T("(a<b) ? (b>c) ? 1 : 2 : 3"), 2., true);
    iNumErr += EqnTest(_T("(a>b) ? (b<c) ? 1 : 2 : 3"), 3., true);
    iNumErr += EqnTest(_T("(a>b) ? (b>c) ? 1 : 2 : 3"), 3., true);

    // Neue Tests
    iNumErr += EqnTest(_T("(a<b)&&(a<b) ? 128 : 255"), 128., true);
    iNumErr += EqnTest(_T("(a>b)&&(a<b) ? 128 : 255"), 255., true);
    iNumErr += EqnTest(_T("(1<2)&&(1<2) ? 128 : 255"), 128., true);
    iNumErr += EqnTest(_T("(1>2)&&(1<2) ? 128 : 255"), 255., true);
    iNumErr += EqnTest(_T("((1<2)&&(1<2)) ? 128 : 255"), 128., true);
    iNumErr += EqnTest(_T("((1>2)&&(1<2)) ? 128 : 255"), 255., true);
    iNumErr += EqnTest(_T("((a<b)&&(a<b)) ? 128 : 255"), 128., true);
    iNumErr += EqnTest(_T("((a>b)&&(a<b)) ? 128 : 255"), 255., true);
    
    iNumErr += EqnTest(_T("1>0 ? 1>2 ? 128 : 255 : 1>0 ? 32 : 64"), 255., true);
    iNumErr += EqnTest(_T("1>0 ? 1>2 ? 128 : 255 :(1>0 ? 32 : 64)"), 255., true);
    iNumErr += EqnTest(_T("1>0 ? 1>0 ? 128 : 255 : 1>2 ? 32 : 64"), 128., true);
    iNumErr += EqnTest(_T("1>0 ? 1>0 ? 128 : 255 :(1>2 ? 32 : 64)"), 128., true);
    iNumErr += EqnTest(_T("1>2 ? 1>2 ? 128 : 255 : 1>0 ? 32 : 64"), 32., true);
    iNumErr += EqnTest(_T("1>2 ? 1>0 ? 128 : 255 : 1>2 ? 32 : 64"), 64., true);
    iNumErr += EqnTest(_T("1>0 ? 50 :  1>0 ? 128 : 255"), 50., true);
    iNumErr += EqnTest(_T("1>0 ? 50 : (1>0 ? 128 : 255)"), 50., true);
    iNumErr += EqnTest(_T("1>0 ? 1>0 ? 128 : 255 : 50"), 128., true);
    iNumErr += EqnTest(_T("1>2 ? 1>2 ? 128 : 255 : 1>0 ? 32 : 1>2 ? 64 : 16"), 32., true);
    iNumErr += EqnTest(_T("1>2 ? 1>2 ? 128 : 255 : 1>0 ? 32 :(1>2 ? 64 : 16)"), 32., true);
    iNumErr += EqnTest(_T("1>0 ? 1>2 ? 128 : 255 :  1>0 ? 32 :1>2 ? 64 : 16"), 255., true);
    iNumErr += EqnTest(_T("1>0 ? 1>2 ? 128 : 255 : (1>0 ? 32 :1>2 ? 64 : 16)"), 255., true);
    iNumErr += EqnTest(_T("true ? false ? 128 : 255 : true ? 32 : 64"), 255., true);

    // assignment operators
    iNumErr += EqnTest(_T("a= false ? 128 : 255"), 255., true);
    iNumErr += EqnTest(_T("a=((a>b)&&(a<b)) ? 128 : 255"), 255., true);
    iNumErr += EqnTest(_T("c=(a<b)&&(a<b) ? 128 : 255"), 128., true);

    iNumErr += EqnTest(_T("a=true?b=true?3:4:5"), 3., true);
    iNumErr += EqnTest(_T("a=false?b=true?3:4:5"), 5., true);

    iNumErr += EqnTest(_T("a=true?5:b=true?3:4"), 5., true);
    iNumErr += EqnTest(_T("a=false?5:b=true?3:4"), 3., true);

    // Issue 42:  	?: operator must be last in expression
    // https://code.google.com/p/muparserx/issues/detail?id=42
    iNumErr += EqnTest(_T("abs(0.1) < 0.25 ? (-1) : (1) + 1"), fabs(0.1) < 0.25 ? (-1.) : (1.) + 1, true);
    iNumErr += EqnTest(_T("abs(a) < 0.25 ? (-1) : (1) + 1"), fabs(a) < 0.25 ? (-1.) : (1.) + 1, true);
    iNumErr += EqnTest(_T("(abs(a) < 0.25 ? -1 : 1)"), (abs(a) < 0.25 ? -1. : 1.), true);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestEqn()
{
    int  iNumErr = 0;
    *m_stream << _T("testing sample equations...");

    iNumErr += ThrowTest(_T("1e1234"), ecUNASSIGNABLE_TOKEN);
    iNumErr += ThrowTest(_T("-1e1234"), ecUNASSIGNABLE_TOKEN);

    iNumErr += EqnTest(_T("-2--8"), (float_type)6.0, true);
    iNumErr += EqnTest(_T("2*(a=9)*3"), 54., true);

    // Functions
    iNumErr += EqnTest(_T("10*strlen(toupper(\"12345\"))"), 50., true);

    // hex value recognition
    iNumErr += EqnTest(_T("0xff"), 255., true);
    iNumErr += EqnTest(_T("10+0xff"), 265., true);
    iNumErr += EqnTest(_T("0xff+10"), 265., true);
    iNumErr += EqnTest(_T("10*0xff"), 2550., true);
    iNumErr += EqnTest(_T("0xff*10"), 2550., true);
    iNumErr += EqnTest(_T("10+0xff+1"), 266., true);
    iNumErr += EqnTest(_T("1+0xff+10"), 266., true);

    // ...
    iNumErr += EqnTest(_T("exp(ln(7))"), 7., true);
    iNumErr += EqnTest(_T("e^ln(7)"), 7., true);
    iNumErr += EqnTest(_T("e^(ln(7))"), 7., true);
    iNumErr += EqnTest(_T("(e^(ln(7)))"), 7., true);
    iNumErr += EqnTest(_T("1-(e^(ln(7)))"), -6., true);
    iNumErr += EqnTest(_T("2*(e^(ln(7)))"), 14., true);
    iNumErr += EqnTest(_T("10^log10(5)"), 5., true);
    iNumErr += EqnTest(_T("10^log10(5)"), 5., true);
    iNumErr += EqnTest(_T("2^log2(4)"), (float_type)4.0, true);
    iNumErr += EqnTest(_T("-(sin(0)+1)"), (float_type)-1.0, true);
    iNumErr += EqnTest(_T("-(2^1.1)"), (float_type)-2.14354692, true);

    // infix + postfix operator in arguments for binary operators (Reference: Matlab)
    iNumErr += EqnTest(_T("-sin(8)m*6"), (float_type)-0.00593615, true);
    iNumErr += EqnTest(_T("-sin(8)m/6"), (float_type)-1.6489e-4, true);
    iNumErr += EqnTest(_T("-sin(8)m+6"), (float_type)5.99901, true);
    iNumErr += EqnTest(_T("-sin(8)m-6"), (float_type)-6.000989, true);

    iNumErr += EqnTest(_T("(cos(2.41)/b)"), (float_type)-0.372056, true);

    // long formula (Reference: Matlab)
    iNumErr += EqnTest(
        _T("(((-9))-e/(((((((pi-(((-7)+(-3)/4/e))))/(((-5))-2)-((pi+(-0))*(sqrt((e+e))*(-8))*(((-pi)+(-pi)-(-9)*(6*5))")
        _T("/(-e)-e))/2)/((((sqrt(2/(-e)+6)-(4-2))+((5/(-2))/(1*(-pi)+3))/8)*pi*((pi/((-2)/(-6)*1*(-1))*(-6)+(-e)))))/")
        _T("((e+(-2)+(-e)*((((-3)*9+(-e)))+(-9)))))))-((((e-7+(((5/pi-(3/1+pi)))))/e)/(-5))/(sqrt((((((1+(-7))))+((((-")
        _T("e)*(-e)))-8))*(-5)/((-e)))*(-6)-((((((-2)-(-9)-(-e)-1)/3))))/(sqrt((8+(e-((-6))+(9*(-9))))*(((3+2-8))*(7+6")
        _T("+(-5))+((0/(-e)*(-pi))+7)))+(((((-e)/e/e)+((-6)*5)*e+(3+(-5)/pi))))+pi))/sqrt((((9))+((((pi))-8+2))+pi))/e")
        _T("*4)*((-5)/(((-pi))*(sqrt(e)))))-(((((((-e)*(e)-pi))/4+(pi)*(-9)))))))+(-pi)"), (float_type)-12.23016549, true);

    // long formula (Reference: Matlab)
    iNumErr += EqnTest(_T("1+2-3*4/5^6*(2*(1-5+(3*7^9)*(4+6*7-3)))+12"), (float_type)-7995810.09926, true);

    /* <ibg 20100321 atan currently unsupported/>

    // long formula (Reference: Matlab)
    iNumErr += EqnTest(
    "(atan(sin((((((((((((((((pi/cos((a/((((0.53-b)-pi)*e)/b))))+2.51)+a)-0.54)/0.98)+b)*b)+e)/a)+b)+a)+b)+pi)/e"
    ")+a)))*2.77)", -2.16995656, true);
    */

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestScript()
{
    int  iNumErr = 0;
    *m_stream << _T("testing script features...");

    // Test error detection
    iNumErr += ThrowTest(_T("sin(\n"), ecUNEXPECTED_NEWLINE);
    iNumErr += ThrowTest(_T("1+\n"), ecUNEXPECTED_NEWLINE);
    iNumErr += ThrowTest(_T("a*\n"), ecUNEXPECTED_NEWLINE);
    iNumErr += ThrowTest(_T("va[\n"), ecUNEXPECTED_NEWLINE);
    iNumErr += ThrowTest(_T("(true) ? \n"), ecUNEXPECTED_NEWLINE);
    iNumErr += ThrowTest(_T("(true) ? 10:\n"), ecUNEXPECTED_NEWLINE);

    // Expressions spanning multiple lines
    iNumErr += EqnTest(_T("a=1\n")
        _T("b=2\n")
        _T("c=3\n")
        _T("a+b+c"), 6., true);

    // Ending an expression with a newline
    iNumErr += EqnTest(_T("3\n"), 3., true);
    iNumErr += EqnTest(_T("1+2\n"), 3., true);
    iNumErr += EqnTest(_T("\n1+2\n"), 3., true);
    iNumErr += EqnTest(_T("\n1+2\n\na+b"), 3., true);

    // Testing comments
    /* 20130107 Not yet...
        iNumErr += EqnTest(_T("a=10 % this is a comment\n")
        _T("b=23 % this is another comment\n")
        _T("a+b"), 33, true);
        */
    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
int ParserTester::TestValReader()
{
    int  iNumErr = 0;
    *m_stream << _T("testing value reader...");

    // Hex value reader
    iNumErr += EqnTest(_T("0x1"), 1., true);
    iNumErr += EqnTest(_T("0x1+0x2"), 3., true);
    iNumErr += EqnTest(_T("0xff"), 255., true);

    // Reading of binary values
    iNumErr += EqnTest(_T("0b1"), 1., true);
    iNumErr += EqnTest(_T("0b01"), 1., true);
    iNumErr += EqnTest(_T("0b11"), 3., true);
    iNumErr += EqnTest(_T("0b011"), 3., true);
    iNumErr += EqnTest(_T("0b11111111"), 255., true);
    iNumErr += EqnTest(_T("b*0b011"), 6.0, true);
    iNumErr += EqnTest(_T("0b1111111111111111111111111111111"), 2147483647., true);
    iNumErr += EqnTest(_T("0b10000000000000000000000000000000"), -2147483647. - 1, true);
    iNumErr += EqnTest(_T("0b11111111111111111111111111111111"), -1., true);
    iNumErr += ThrowTest(_T("0b100000000000000000000000000000000"), ecUNDEFINED);

    // string value reader
    iNumErr += EqnTest(_T("\"hallo\""), _T("hallo"), true);

    // boolean value reader
    iNumErr += EqnTest(_T("true"), true, true);
    iNumErr += EqnTest(_T("false"), false, true);

    // boolean value reader
    iNumErr += EqnTest(_T("true"), true, true);
    iNumErr += EqnTest(_T("false"), false, true);

    // mixed
    iNumErr += EqnTest(_T("0b011+0xef"), 242., true);

    Assessment(iNumErr);
    return iNumErr;
}

//---------------------------------------------------------------------------
void ParserTester::AddTest(testfun_type a_pFun)
{
    m_vTestFun.push_back(a_pFun);
}

//---------------------------------------------------------------------------
void ParserTester::Run()
{
    int iStat = 0;
    try
    {
        for (int i = 0; i < (int)m_vTestFun.size(); ++i)
            iStat += (this->*m_vTestFun[i])();
    }
    catch (ParserError &e)
    {
        *m_stream << e.GetMsg() << endl;
        *m_stream << e.GetToken() << endl;
        Abort();
    }
    catch (std::exception &e)
    {
        *m_stream << e.what() << endl;
        Abort();
    }
    catch (...)
    {
        *m_stream << _T("Internal error");
        Abort();
    }

    if (iStat == 0)
    {
        *m_stream << _T("Test passed (") << ParserTester::c_iCount << _T(" expressions)") << endl;
    }
    else
    {
        *m_stream << _T("Test failed with ") << iStat
            << _T(" errors (") << ParserTester::c_iCount
            << _T(" expressions)") << endl;
    }
    ParserTester::c_iCount = 0;

#ifdef MUP_LEAKAGE_REPORT
    // All tokens must have been destroyed by now, verify this
    IToken::LeakageReport();
#endif
}


//---------------------------------------------------------------------------
int ParserTester::ThrowTest(const string_type &a_sExpr, int a_nErrc, int a_nPos, string_type a_sIdent)
{
    ParserTester::c_iCount++;

    try
    {
        ParserX p;

        // Add variables
        Value vVarVal[] = { 1., 2., 3., -2. };
        p.DefineVar(_T("a"), Variable(&vVarVal[0]));
        p.DefineVar(_T("b"), Variable(&vVarVal[1]));
        p.DefineVar(_T("c"), Variable(&vVarVal[2]));
        p.DefineVar(_T("d"), Variable(&vVarVal[3]));

        // array variables
        Value aVal1(3, 0);
        aVal1.At(0) = (float_type)1.0;
        aVal1.At(1) = (float_type)2.0;
        aVal1.At(2) = (float_type)3.0;

        Value aVal2(3, 0);
        aVal2.At(0) = (float_type)4.0;
        aVal2.At(1) = (float_type)3.0;
        aVal2.At(2) = (float_type)2.0;

        Value aVal3(4, 0);
        aVal3.At(0) = (float_type)4.0;
        aVal3.At(1) = (float_type)3.0;
        aVal3.At(2) = (float_type)2.0;
        aVal3.At(3) = (float_type)5.0;

        Value aVal4(4, 0);
        aVal4.At(0) = (float_type)4.0;
        aVal4.At(1) = false;
        aVal4.At(2) = _T("hallo");

        // Matrix variables
        Value m1(3, 3, 0);
        m1.At(0, 0) = 1.;
        m1.At(1, 1) = 1.;
        m1.At(2, 2) = 1.;

        Value m2(3, 3, 0);
        m2.At(0, 0) = 1.;  m2.At(0, 1) = 2.;  m2.At(0, 2) = 3.;
        m2.At(1, 0) = 4.;  m2.At(1, 1) = 5.;  m2.At(1, 2) = 6.;
        m2.At(2, 0) = 7.;  m2.At(2, 1) = 8.;  m2.At(2, 2) = 9.;

        p.DefineVar(_T("m1"), Variable(&m1));
        p.DefineVar(_T("m2"), Variable(&m2));
        p.DefineVar(_T("va"), Variable(&aVal1));
        p.DefineVar(_T("vb"), Variable(&aVal2));
        p.DefineVar(_T("vc"), Variable(&aVal3));
        p.DefineVar(_T("vd"), Variable(&aVal4));

        p.SetExpr(a_sExpr);
        Value fRes = p.Eval();
    }
    catch (ParserError &e)
    {
        // output the formula in case of an failed test
        if (a_nErrc != e.GetCode())
        {
            *m_stream << _T("\n  ")
                << _T("Expression: \"") << a_sExpr
                << _T("\"  Code:") << e.GetCode()
                << _T("  Expected:") << a_nErrc;
        }

        // Check whether the error is reported at the correct expression position
        if (a_nPos != -1 && a_nPos != e.GetPos())
        {
            *m_stream << _T("\n  ")
                << _T("Invalid error position: \"") << a_sExpr
                << _T("\"  Pos:") << e.GetPos()
                << _T("  Expected:") << a_nPos;
        }

        if (a_sIdent.length() && a_sIdent != e.GetContext().Ident)
        {
            *m_stream << _T("\n  ")
                << _T("Invalid identifier: \"") << a_sExpr
                << _T("\"  Ident:") << e.GetContext().Ident
                << _T("  Expected:") << a_sIdent;
        }

        return (a_nErrc == e.GetCode() && (a_nPos == -1 || a_nPos == e.GetPos())) ? 0 : 1;
    }

    *m_stream << _T("\n  ")
        << _T("Expression failed: \"")
        << a_sExpr
        << _T("\"  (no exception raised).");

    return 1;
}

//---------------------------------------------------------------------------
int ParserTester::EqnTest(const string_type &a_str, Value a_val, bool a_fPass, int nExprVar)
{
    ParserTester::c_iCount++;
    int iRet(1);
    Value fVal[5];

    try
    {
        // p1 is a pointer since I'm going to delete it in order to test if
        // parsers after copy construction still refer to members of the deleted object.
        // !! If this is the case this function will crash !!
        std::unique_ptr<ParserX> p1(new ParserX());

        // Add variables
        Value vVarVal[] = { 1., 2., 3., -2., -1. };

        // m1 ist die Einheitsmatrix
        Value m1(3, 3, 0);
        m1.At(0, 0) = 1.;
        m1.At(1, 1) = 1.;
        m1.At(2, 2) = 1.;

        // m2 ist die Einheitsmatrix
        Value m2(3, 3, 0);
        m2.At(0, 0) = 1.;  m2.At(0, 1) = 2.;  m2.At(0, 2) = 3.;
        m2.At(1, 0) = 4.;  m2.At(1, 1) = 5.;  m2.At(1, 2) = 6.;
        m2.At(2, 0) = 7.;  m2.At(2, 1) = 8.;  m2.At(2, 2) = 9.;

        p1->DefineOprt(new DbgSillyAdd);
        p1->DefineFun(new FunTest0);

        p1->DefineVar(_T("a"), Variable(&vVarVal[0]));
        p1->DefineVar(_T("b"), Variable(&vVarVal[1]));
        p1->DefineVar(_T("c"), Variable(&vVarVal[2]));
        p1->DefineVar(_T("d"), Variable(&vVarVal[3]));
        p1->DefineVar(_T("f"), Variable(&vVarVal[4]));
        p1->DefineVar(_T("m1"), Variable(&m1));
        p1->DefineVar(_T("m2"), Variable(&m2));

        // Add constants
        p1->DefineConst(_T("const"), 1.);
        p1->DefineConst(_T("const1"), 2.);
        p1->DefineConst(_T("const2"), 3.);

        // some vector variables
        Value aVal1(3, 0);
        aVal1.At(0) = (float_type)1.0;
        aVal1.At(1) = (float_type)2.0;
        aVal1.At(2) = (float_type)3.0;

        Value aVal2(3, 0);
        aVal2.At(0) = (float_type)4.0;
        aVal2.At(1) = (float_type)3.0;
        aVal2.At(2) = (float_type)2.0;
        p1->DefineVar(_T("va"), Variable(&aVal1));
        p1->DefineVar(_T("vb"), Variable(&aVal2));

        // complex variables
        Value cVal[3];
        cVal[0] = mup::cmplx_type(1, 1);
        cVal[1] = mup::cmplx_type(2, 3);
        cVal[2] = mup::cmplx_type(3, 4);
        p1->DefineVar(_T("ca"), Variable(&cVal[0]));
        p1->DefineVar(_T("cb"), Variable(&cVal[1]));
        p1->DefineVar(_T("cc"), Variable(&cVal[2]));

        p1->SetExpr(a_str);

        fVal[0] = p1->Eval();

        // Test copy and assignement operators
        std::vector<ParserX> vParser;
        vParser.push_back(*p1);   // Push p1 into the vector
        ParserX p2 = vParser[0];   // take parser from vector

        // destroy the originals from p2
        vParser.clear();              // delete the vector
        p1.reset(0);                  // delete the original

        fVal[1] = p2.Eval();          // If copy constructions does not work
        // we may see a crash here

        // Test assignement operator
        // additionally  disable Optimizer this time
        ParserX p3;
        p3 = p2;
        fVal[2] = p3.Eval();          // If assignment does not work
        // we may see a crash here

        // Calculating a second time will parse from rpn rather than from
        // string. The result must be the same...
        fVal[3] = p3.Eval();

        // Calculate yet another time. There is the possibility of
        // changing variables as a side effect of expression
        // evaluation. So there are really bugs that could make this fail...
        fVal[4] = p3.Eval();

        // Check i number of used variables is correct
        if (nExprVar != -1)
        {
            int n2 = p2.GetExprVar().size();
            int n3 = p3.GetExprVar().size();

            if (n2 + n3 != 2 * n2 || n2 != nExprVar)
            {
                *m_stream << _T("  Number of expression variables is incorrect. (expected: ")
                    << nExprVar << _T("; detected: ") << n2 << _T(")");
            }
        }

        // Check the three results
        // 1.) computed results must have identic type
        char_type cType = fVal[0].GetType();
        bool bStat = cType == fVal[1].GetType() &&
            cType == fVal[2].GetType() &&
            cType == fVal[3].GetType() &&
            cType == fVal[4].GetType();
        if (!bStat)
        {
            *m_stream << _T("\n  ") << a_str << _T(" :  inconsistent result type (")
                      << fVal[0].GetType() << _T(", ")
                      << fVal[1].GetType() << _T(", ")
                      << fVal[2].GetType() << _T(", ")
                      << fVal[3].GetType() << _T(", ")
                      << fVal[4].GetType() << _T(")");
            return 1;
        }

        if ((cType == 'c' || a_val.GetType() == 'c') && cType != a_val.GetType())
        {
            *m_stream << _T("\n  ") << a_str << _T(" :  Complex value sliced!");
            return 1;
        }

        // Compare the results
        switch (cType)
        {
        case  'i':
        case  'b':
        case  's': bStat = (a_val == fVal[0] &&
            a_val == fVal[1] &&
            a_val == fVal[2] &&
            a_val == fVal[3] &&
            a_val == fVal[4]);
            break;

            // We need more attention for comaring float values due to floating point
            // inaccuracies.
        case  'f':
        {
                     bStat = true;
                     int num = sizeof(fVal) / sizeof(Value);
                     for (int i = 0; i < num; ++i)
                         bStat &= (fabs(a_val.GetFloat() - fVal[i].GetFloat()) <= fabs(fVal[i].GetFloat()*0.0001));
        }
            break;

        case 'c':
        {
                    bStat = true;
                    int num = sizeof(fVal) / sizeof(Value);
                    for (int i = 0; i < num; ++i)
                    {
                        bStat &= (fabs(a_val.GetFloat() - fVal[i].GetFloat()) <= std::max((float_type)1e-15, fabs(fVal[i].GetFloat() * (float_type)0.0000001)));
                        bStat &= (fabs(a_val.GetImag() - fVal[i].GetImag()) <= std::max((float_type)1e-15, fabs(fVal[i].GetImag()  * (float_type)0.0000001)));
                    }
        }
            break;

        case 'm':
        {
                    bStat = true;
                    int num = sizeof(fVal) / sizeof(Value);

                    for (int i = 0; i < num; ++i)
                    {
                        struct CheckArray
                        {
                            CheckArray()
                            {}

                            bool Check(IValue &v1, IValue &v2)
                            {
                                if (v1.GetType() != v2.GetType())
                                    return false;

                                if (v1.GetRows() != v2.GetRows())
                                    return false;

                                if (v1.IsMatrix())
                                {
                                    for (int i = 0; i < v1.GetRows(); ++i)
                                    {
                                        for (int j = 0; j < v1.GetCols(); ++j)
                                        {
                                            if (!Check(v1.At(i, j), v2.At(i, j)))
                                                return false;
                                        }
                                    }

                                    return true;
                                }
                                else
                                {
                                    return (fabs(v1.GetFloat() - v2.GetFloat()) <= std::max((float_type)1e-15, fabs(v1.GetFloat() * (float_type)0.0000001)));
                                }
                            }
                        } checker;

                        bStat = checker.Check(a_val, fVal[i]);
                        if (!bStat)
                            break;
                    }
        }
            break;

        default:
            throw std::runtime_error("Parser return value has an unexpected typecode.");
        }

        iRet = (bStat == a_fPass) ? 0 : 1;
    }
    catch (ParserError &e)
    {
        *m_stream << _T("\n  ") << a_str << _T(" : ") << e.GetMsg();
        return 1;
    }
    catch (std::exception &e)
    {
        *m_stream << _T("\n  ") << a_str << _T(" : ") << e.what() << _T("\n");
        return 1;
    }
    catch (...)
    {
        *m_stream << _T("\n  \"") << a_str << _T("\" : ") << _T("Unexpected Eception");
        return 1;
    }

    if (iRet)
    {
        *m_stream << _T("\n  ") << a_str << _T(" : ") << _T("(at least one incorrect result ")
            << fVal[0] << _T(", ")
            << fVal[1] << _T(", ")
            << fVal[2] << _T(", ")
            << fVal[3] << _T(", ")
            << fVal[4] << _T("; expected=") << a_val << _T(")");
    }

    return iRet;
}

//---------------------------------------------------------------------------
/** \brief Internal error in test class Test is going to be aborted. */
void ParserTester::Abort() const
{
    *m_stream << _T("\nTest failed (internal error in test class)") << endl;
    while (!getchar());
    exit(-1);
}

//---------------------------------------------------------------------------
void ParserTester::Assessment(int a_iNumErr) const
{
    if (a_iNumErr == 0)
        *m_stream << _T("passed") << endl;
    else
        *m_stream << _T("\n  failed with ") << a_iNumErr << _T(" errors") << endl;
}
MUP_NAMESPACE_END
