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
