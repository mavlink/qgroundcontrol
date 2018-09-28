#ifndef MUP_STACK_H
#define MUP_STACK_H

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

#include <cassert>
#include <string>
#include <vector>

#include "mpError.h"


MUP_NAMESPACE_START

  /** \brief Parser stack implementation. 

  Stack implementation based on a std::stack. The behaviour of pop() had been
  slightly changed in order to get an error code if the stack is empty.
  The stack is used within the Parser both as a value stack and as an operator stack.

  \author (C) 2010 Ingo Berg 
  */
  template <typename TVal, typename TCont = std::vector<TVal> >
  class Stack 
  {
  private:
      /** \brief Type of the underlying container. */
      typedef TCont cont_type;
      cont_type m_Cont;

  public:	
      typedef TVal value_type;
    
      //---------------------------------------------------------------------------
      Stack()
        :m_Cont()
      {}

      //---------------------------------------------------------------------------
      virtual ~Stack()
      {
        m_Cont.clear();
      }

      //---------------------------------------------------------------------------
      void clear()
      {
        m_Cont.clear();
      }

      //---------------------------------------------------------------------------
      /** \brief Pop a value from the stack.
      
        Unlike the standard implementation this function will return the value that
        is going to be taken from the stack.

        \throw ParserException in case the stack is empty.
        \sa pop(int &a_iErrc)
      */
      value_type pop()
      {
        if (empty())
          throw ParserError(_T("stack is empty."));

        value_type el = top();
        m_Cont.pop_back();
        return el;
      }

      //---------------------------------------------------------------------------
      void pop(unsigned a_iNum)
      {
        for (unsigned i=0; i<a_iNum; ++i)
          m_Cont.pop_back();
      }

      //---------------------------------------------------------------------------
      /** \brief Push an object into the stack. 
          \param a_Val object to push into the stack.
          \throw nothrow
      */
      void push(const value_type& a_Val) 
      { 
        m_Cont.push_back(a_Val); 
      }

      //---------------------------------------------------------------------------
      /** \brief Return the number of stored elements. */
      unsigned size() const
      { 
        return (unsigned)m_Cont.size(); 
      }

      //---------------------------------------------------------------------------
      /** \brief Returns true if stack is empty false otherwise. */
      bool empty() const
      {
        return m_Cont.empty(); 
      }

      //---------------------------------------------------------------------------
      /** \brief Return reference to the top object in the stack. 
      
          The top object is the one pushed most recently.
      */
      value_type& top() 
      { 
        return m_Cont.back(); 
      }

      //---------------------------------------------------------------------------
      value_type* get_data()
      {
        return &m_Cont[0];
      }

      //---------------------------------------------------------------------------
      const value_type* get_data() const
      {
        return &m_Cont[0];
      }
  };
} // namespace mu

#endif
