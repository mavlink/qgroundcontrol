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
 *   @brief Definition of the class WebImage.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#ifndef WEBIMAGE_H
#define WEBIMAGE_H

#include <inttypes.h>
#include <QImage>
#include <QScopedPointer>
#include <QSharedPointer>

class WebImage
{
public:
    WebImage();

    void clear(void);

    enum State
    {
        UNINITIALIZED = 0,
        REQUESTED = 1,
        READY = 2
    };

    State getState(void) const;
    void setState(State state);

    const QString& getSourceURL(void) const;
    void setSourceURL(const QString& url);

    const uint8_t* getData(void) const;
    void setData(const QByteArray& data);

    int32_t getWidth(void) const;
    int32_t getHeight(void) const;
    int32_t getByteCount(void) const;

    uint64_t getLastReference(void) const;
    void setLastReference(uint64_t value);

    bool getSyncFlag(void) const;
    void setSyncFlag(bool onoff);

private:
    State state;
    QString sourceURL;
    QScopedPointer<QImage> image;
    uint64_t lastReference;
    bool syncFlag;
};

typedef QSharedPointer<WebImage> WebImagePtr;

#endif // WEBIMAGE_H
