/*
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

#ifndef QTQUICK2VIDEOSINKDELEGATE_H
#define QTQUICK2VIDEOSINKDELEGATE_H

#include "basedelegate.h"
#include <QtQuick/QSGNode>

class QtQuick2VideoSinkDelegate : public BaseDelegate
{
    Q_OBJECT
public:
    explicit QtQuick2VideoSinkDelegate(GstElement * sink, QObject * parent = 0);

    QSGNode *updateNode(QSGNode *node, const QRectF & targetArea);
};

#endif // QTQUICK2VIDEOSINKDELEGATE_H
