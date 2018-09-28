#ifndef MUP_VALUE_CACHE_H
#define MUP_VALUE_CACHE_H

/** \file
    \brief Implementation of a cache for recycling unused value items.

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
#include <vector>

#include "mpFwdDecl.h"


MUP_NAMESPACE_START
  
  /** \brief The ValueCache class provides a simple mechanism to recycle 
             unused value items.
    
    This class serves as a factory for value items. It allows skipping
    unnecessary and slow new/delete calls by storing unused value 
    objects in an internal buffer for later reuse. By eliminating new/delete
    calls the parser is sped up approximately by factor 3-4.
  */
  class ValueCache
  {
  public:
    ValueCache(int size=10);
   ~ValueCache();

    void ReleaseAll();
    void ReleaseToCache(Value *pValue);
    Value* CreateFromCache();

  private:
    ValueCache(const ValueCache &ref);
    ValueCache& operator=(const ValueCache &ref);

    int m_nIdx;
    std::vector<Value*> m_vCache;
  };

MUP_NAMESPACE_END

#endif // include guard