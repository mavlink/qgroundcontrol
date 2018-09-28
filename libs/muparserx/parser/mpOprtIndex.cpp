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
#include "mpOprtIndex.h"
#include "mpVariable.h"

MUP_NAMESPACE_START

    //-----------------------------------------------------------------------------------------------
    //
    //  class  OprtIndex
    //
    //-----------------------------------------------------------------------------------------------

    OprtIndex::OprtIndex()
        :ICallback(cmIC, _T("Index operator"), -1)
    {}

    //-----------------------------------------------------------------------------------------------
    /** \brief Index operator implementation
    \param ret A reference to the return value
    \param a_pArg Pointer to an array with the indices as ptr_val_type
    \param a_iArgc Number of indices (=dimension) actully used in the expression found. This must 
            be 1 or 2 since three dimensional data structures are not supported by muParserX.
    */
    void OprtIndex::Eval(ptr_val_type &ret, const ptr_val_type *a_pArg, int a_iArgc)
    {
        try
        {
            int rows = a_pArg[-1]->GetRows();
            int cols = a_pArg[-1]->GetCols();
            bool bArgIsVariable = a_pArg[-1]->IsVariable();

            // If the index operator is applied to a variable the return value is also a variable
            // pointing to a specific cell in the matrix. If the operator is applied to a value
            // the return value is also a value.
            switch (a_iArgc)
            {
            case 1:
                if (cols == 1)
                {
                    if (bArgIsVariable) 
                        ret.Reset(new Variable(&(ret->At(*a_pArg[0], Value(0.0)))));
                    else
                        *ret = ret->At(*a_pArg[0], Value(0.0));
                }
                else if (rows == 1)
                {
                    if (bArgIsVariable) 
                        ret.Reset(new Variable(&(ret->At(Value(0.0), *a_pArg[0]))));
                    else
                        *ret = ret->At(Value(0.0), *a_pArg[0]);
                }
                else
                {
                    throw ParserError(ErrorContext(ecINDEX_DIMENSION, -1, GetIdent()));
                }
                break;

            case 2:
                if (bArgIsVariable)
                    ret.Reset(new Variable(&(ret->At(*a_pArg[0], *a_pArg[1]))));
                else
	                *ret = ret->At(*a_pArg[0], *a_pArg[1]);
                break;

            default:
                throw ParserError(ErrorContext(ecINDEX_DIMENSION, -1, GetIdent()));
            }
        }
        catch(ParserError &exc)
        {
            exc.GetContext().Pos = GetExprPos();
            throw exc;
        }
    }

    //-----------------------------------------------------------------------------------------------
    const char_type* OprtIndex::GetDesc() const
    {
        return _T("[,] - The index operator.");
    }

    //-----------------------------------------------------------------------------------------------
    IToken* OprtIndex::Clone() const
    {
        return new OprtIndex(*this); 
    }

MUP_NAMESPACE_END
