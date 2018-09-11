/** \file
    \brief Definition of a class for caching unused value items and recycling them.

<pre>
               __________                                 ____  ___
    _____  __ _\______   \_____ _______  ______ __________\   \/  /
   /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     / 
  |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \ 
  |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
        \/                     \/           \/     \/           \_/

  muParserX - A C++ math parser library with array and string support
  Copyright 2010 Ingo Berg

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE
  as published by the Free Software Foundation, either version 3 of 
  the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see http://www.gnu.org/licenses.
</pre>
*/
#include "mpValueCache.h"

#include "mpValue.h"


MUP_NAMESPACE_START

  //------------------------------------------------------------------------------
  ValueCache::ValueCache(int size)
    :m_nIdx(-1)
    ,m_vCache(size, (mup::Value*)0) // hint to myself: don't use NULL gcc will go postal...
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
      m_vCache[i] = NULL;
    }

    m_nIdx = -1;
  }

  //------------------------------------------------------------------------------
  void ValueCache::ReleaseToCache(Value *pValue) 
  {
//    std::cout << "dbg: " << ct << " ptr: " << this << " void ValueCache::ReleaseToCache(Value *pValue) \n";
    if (pValue==NULL)
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
    Value *pValue = NULL;
    if (m_nIdx>=0)
    {
      pValue = m_vCache[m_nIdx];
      m_vCache[m_nIdx] = NULL;
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
