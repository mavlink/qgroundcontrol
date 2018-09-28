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

#include <iostream>
#include <iomanip>

#include "mpRPN.h"
#include "mpIToken.h"
#include "mpICallback.h"
#include "mpError.h"
#include "mpStack.h"
#include "mpIfThenElse.h"
#include "mpScriptTokens.h"


MUP_NAMESPACE_START

  //---------------------------------------------------------------------------
  RPN::RPN()
    :m_vRPN()
    ,m_nStackPos(-1)
    ,m_nLine(0)
    ,m_nMaxStackPos(0)
    ,m_bEnableOptimizer(false)
  {}
  
  //---------------------------------------------------------------------------
  RPN::~RPN()
  {}
    
  //---------------------------------------------------------------------------
  void RPN::Add(ptr_tok_type tok)
  {
    m_vRPN.push_back(tok);
    if (tok->AsIValue()!=nullptr)
    {
      m_nStackPos++;
    }
    else if (tok->AsICallback())
    {
      ICallback *pFun = tok->AsICallback();
      MUP_VERIFY(pFun!=nullptr);
      m_nStackPos -= pFun->GetArgsPresent() - 1;
    }

    MUP_VERIFY(m_nStackPos>=0);
    m_nMaxStackPos = std::max(m_nStackPos, m_nMaxStackPos);
  }
  
  //---------------------------------------------------------------------------
  void RPN::AddNewline(ptr_tok_type tok, int n)
  {
    static_cast<TokenNewline*>(tok.Get())->SetStackOffset(n);
    m_vRPN.push_back(tok);
    m_nStackPos -= n;
    m_nLine++;
  }

  //---------------------------------------------------------------------------
  void RPN::Pop(int num)
  {
    if (m_vRPN.size()==0)
      return;

    for (int i=0; i<num; ++i)
    {
      ptr_tok_type tok = m_vRPN.back();
      
      if (tok->AsIValue()!=0)
        m_nStackPos--;

      m_vRPN.pop_back();
    }
  }

  //---------------------------------------------------------------------------
  void RPN::Reset()
  {
    m_vRPN.clear();
    m_nStackPos = -1;
    m_nMaxStackPos = 0;
    m_nLine = 0;
  }

  //---------------------------------------------------------------------------
  /** \brief 

      At the moment this will only ass the jump distances to the if-else clauses 
      found in the expression. 
  */
  void RPN::Finalize()
  {
    // Determine the if-then-else jump offsets
    Stack<int> stIf, stElse;
    int idx;
    for (std::size_t i=0; i<m_vRPN.size(); ++i)
    {
      switch(m_vRPN[i]->GetCode())
      {
      case  cmIF:
            stIf.push(i);
            break;

      case  cmELSE:
            stElse.push(i);
            idx = stIf.pop();
            static_cast<TokenIfThenElse*>(m_vRPN[idx].Get())->SetOffset(i - idx);
            break;
      
      case  cmENDIF:
            idx = stElse.pop();
            static_cast<TokenIfThenElse*>(m_vRPN[idx].Get())->SetOffset(i - idx);
            break;
			
	  default:
            continue;
      }
    }
  }

  //---------------------------------------------------------------------------
  void  RPN::EnableOptimizer(bool bStat)
  {
    m_bEnableOptimizer = bStat;
  }

  //---------------------------------------------------------------------------
  std::size_t RPN::GetSize() const
  {
    return m_vRPN.size();
  }

  //---------------------------------------------------------------------------
  const token_vec_type& RPN::GetData() const
  {
    return m_vRPN;
  }

  //---------------------------------------------------------------------------
  int RPN::GetRequiredStackSize() const
  {
    return m_nMaxStackPos + 1;
  }

  //---------------------------------------------------------------------------
  void RPN::AsciiDump() const
  {
    console() << "Number of tokens: " << m_vRPN.size() << "\n";
    console() << "MaxStackPos:       " << m_nMaxStackPos << "\n";
    for (std::size_t i=0; i<m_vRPN.size(); ++i)
    {
      ptr_tok_type pTok = m_vRPN[i];
      console() << std::setw(2) << i << " : "
                << std::setw(2) << pTok->GetExprPos() << " : " 
                << pTok->AsciiDump() << std::endl;
    }
  }

MUP_NAMESPACE_END
