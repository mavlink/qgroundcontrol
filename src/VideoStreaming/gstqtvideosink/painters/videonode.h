/*
    Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
    Copyright (C) 2013 basysKom GmbH <info@basyskom.com>
    Copyright (C) 2013 Collabora Ltd. <info@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 2.1
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 *   @brief Extracted from QtGstreamer to avoid overly complex dependency
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEONODE_H
#define VIDEONODE_H

#include "../utils/bufferformat.h"

#include <QtQuick/QSGGeometryNode>

class VideoNode : public QSGGeometryNode
{
public:
    VideoNode();

    enum MaterialType {
        MaterialTypeVideo,
        MaterialTypeSolidBlack
    };

    MaterialType materialType() const { return m_materialType; }

    void changeFormat(const BufferFormat &format);
    void setMaterialTypeSolidBlack();

    void setCurrentFrame(GstBuffer *buffer);
    void updateColors(int brightness, int contrast, int hue, int saturation);

    void updateGeometry(const PaintAreas & areas);

private:
    MaterialType m_materialType;
};

#endif // VIDEONODE_H
