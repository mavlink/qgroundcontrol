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
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "TextureCache.h"

TextureCache::TextureCache(uint32_t _cacheSize)
    : cacheSize(_cacheSize)
    , imageCache(new WebImageCache(0, cacheSize))
{
    for (uint32_t i = 0; i < cacheSize; ++i) {
        TexturePtr t(new Texture(i));

        textures.push_back(t);
    }
}

TexturePtr
TextureCache::get(const QString& tileURL)
{
    QPair<TexturePtr, int32_t> p1 = lookup(tileURL);
    if (!p1.first.isNull()) {
        return p1.first;
    }

    QPair<WebImagePtr, int32_t> p2 = imageCache->lookup(tileURL);
    if (!p2.first.isNull()) {
        textures[p2.second]->sync(p2.first);
        p1 = lookup(tileURL);

        return p1.first;
    }

    return TexturePtr();
}

void
TextureCache::sync(void)
{
    if (requireSync()) {
        for (int32_t i = 0; i < textures.size(); ++i) {
            textures[i]->sync(imageCache->at(i));
        }
    }
}

QPair<TexturePtr, int32_t>
TextureCache::lookup(const QString& tileURL)
{
    for (int32_t i = 0; i < textures.size(); ++i) {
        if (textures[i]->getSourceURL() == tileURL) {
            return qMakePair(textures[i], i);
        }
    }

    return qMakePair(TexturePtr(), -1);
}

bool
TextureCache::requireSync(void) const
{
    for (uint32_t i = 0; i < cacheSize; ++i) {
        if (imageCache->at(i)->getSyncFlag()) {
            return true;
        }
    }
    return false;
}
