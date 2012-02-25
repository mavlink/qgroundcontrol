/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of the class TextureCache.
 *
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#include "TextureCache.h"

TextureCache::TextureCache(int cacheSize)
    : mCacheSize(cacheSize)
    , mImageCache(new WebImageCache(0, cacheSize))
{
    for (int i = 0; i < mCacheSize; ++i)
    {
        TexturePtr t(new Texture(i));

        mTextures.push_back(t);
    }
}

TexturePtr
TextureCache::get(const QString& tileURL)
{
    QPair<TexturePtr, int> p1 = lookup(tileURL);
    if (!p1.first.isNull())
    {
        return p1.first;
    }

    QPair<WebImagePtr, int> p2 = mImageCache->lookup(tileURL);
    if (!p2.first.isNull())
    {
        mTextures[p2.second]->sync(p2.first);
        p1 = lookup(tileURL);

        return p1.first;
    }

    return TexturePtr();
}

void
TextureCache::sync(void)
{
    if (requireSync())
    {
        for (int i = 0; i < mTextures.size(); ++i)
        {
            mTextures[i]->sync(mImageCache->at(i));
        }
    }
}

QPair<TexturePtr, int>
TextureCache::lookup(const QString& tileURL)
{
    for (int i = 0; i < mTextures.size(); ++i)
    {
        if (mTextures[i]->getSourceURL() == tileURL)
        {
            return qMakePair(mTextures[i], i);
        }
    }

    return qMakePair(TexturePtr(), -1);
}

bool
TextureCache::requireSync(void) const
{
    for (int i = 0; i < mCacheSize; ++i)
    {
        if (mImageCache->at(i)->getSyncFlag())
        {
            return true;
        }
    }
    return false;
}
