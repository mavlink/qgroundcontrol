/*
    Copyright (C) 2010 George Kiagiadakis <kiagiadakis.george@gmail.com>
    Copyright (C) 2012 Collabora Ltd. <info@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

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

#ifndef QWIDGET_VIDEO_SINK_DELEGATE_H
#define QWIDGET_VIDEO_SINK_DELEGATE_H

#include "qtvideosinkdelegate.h"
#include <QEvent>
#include <QPointer>
#include <QWidget>

class QWidgetVideoSinkDelegate : public QtVideoSinkDelegate
{
    Q_OBJECT
public:
    explicit QWidgetVideoSinkDelegate(GstElement * sink, QObject * parent = 0);
    virtual ~QWidgetVideoSinkDelegate();

    // "widget" property
    QWidget *widget() const;
    void setWidget(QWidget *widget);

protected:
    virtual bool eventFilter(QObject *filteredObject, QEvent *event);
    virtual void update();

private:
    // "widget" property
    QPointer<QWidget> m_widget;

    // original value of the Qt::WA_OpaquePaintEvent attribute
    bool m_opaquePaintEventAttribute;
};

#endif // QWIDGET_VIDEO_SINK_DELEGATE_H
