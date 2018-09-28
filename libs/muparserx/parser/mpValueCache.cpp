/** \file
    \brief Definition of a class for caching unused value items and recycling them.

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
#include "mpValueCache.h"

#include "mpValue.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  ValueCache::ValueCache(int size)
    :m_nIdx(-1)
    ,m_vCache(size, (mup::Value*)0) // hint to myself: don't use nullptr gcc will go postal...
  {}

  //------------------------------------------------------------------------------
  ValueCache::~ValueCache()
  {
    ReleaseAll();
  }

  //------------------------------------------------------------------------------
  void ValueCache::ReleaseAll()
  {
    for (std::size_t i=0; i<m_vCache.size(); ++i)
    {
      delete m_vCache[i];
      m_vCache[i] = nullptr;
    }

    m_nIdx = -1;
  }

  //------------------------------------------------------------------------------
  void ValueCache::ReleaseToCache(Value *pValue) 
  {
//    std::cout << "dbg: " << ct << " ptr: " << this << " void ValueCache::ReleaseToCache(Value *pValue) \n";
    if (pValue==nullptr)
      return;

    assert(pValue->GetRef()==0);

    // Add the value to the cache if the cache has room for it 
    // otherwise release the value item instantly
    if ( m_nIdx < ((int)m_vCache.size()-1) )
    {
      m_nIdx++;
      m_vCache[m_nIdx] = pValue;
    }
    else
      delete pValue;
  }

  //------------------------------------------------------------------------------
  Value* ValueCache::CreateFromCache() 
  {
    Value *pValue = nullptr;
    if (m_nIdx>=0)
    {
      pValue = m_vCache[m_nIdx];
      m_vCache[m_nIdx] = nullptr;
      m_nIdx--;
    }
    else
    {
      pValue = new Value();
      pValue->BindToCache(this);
    }

    return pValue;
  }

MUP_NAMESPACE_END
