/*
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
*/
#ifndef MU_PARSER_TEST_H
#define MU_PARSER_TEST_H

#include <string>
#include <numeric> // for accumulate
#include "mpParser.h"
#include "mpOprtBinCommon.h"


MUP_NAMESPACE_START

    /** \brief Test cases for unit testing the parser framework.
    */
    class ParserTester // final
    {
    private:
        static int c_iCount;

        int TestParserValue();
        int TestErrorCodes();
        int TestStringFun();
        int TestVector();
        int TestBinOp();
        int TestPostfix();
        int TestInfix();
        int TestEqn();
        int TestMultiArg();
        int TestUndefVar();
        int TestIfElse();
        int TestMatrix();
        int TestComplex();
        int TestScript();
		int TestValReader();
        int TestIssueReports();

        void Assessment(int a_iNumErr) const;
        void Abort() const;

    public:
        typedef int (ParserTester::*testfun_type)();

        ParserTester();

        /** \brief Destructor (trivial). */
       ~ParserTester() {};
      	
        /** \brief Copy constructor is deactivated. */
        ParserTester(const ParserTester &a_Obj)
          :m_vTestFun()
          ,m_stream(a_Obj.m_stream)
        {};

        void Run();

    private:
        std::vector<testfun_type> m_vTestFun;

#if defined(_UNICODE)
        std::wostream *m_stream;
#else
        std::ostream *m_stream;
#endif

        void AddTest(testfun_type a_pFun);

        // Test Double Parser
        int EqnTest(const string_type &a_str, Value a_val, bool a_fPass, int nExprVar = -1);
        int ThrowTest(const string_type &a_str, int a_nErrc, int a_nPos = -1, string_type a_sIdent = string_type());
    }; // ParserTester
}  // namespace mu

#endif


