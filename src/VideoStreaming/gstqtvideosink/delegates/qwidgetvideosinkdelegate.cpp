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

#include "qwidgetvideosinkdelegate.h"
#include <QPainter>

QWidgetVideoSinkDelegate::QWidgetVideoSinkDelegate(GstElement * sink, QObject * parent)
    : QtVideoSinkDelegate(sink, parent)
{

}

QWidgetVideoSinkDelegate::~QWidgetVideoSinkDelegate()
{
    setWidget(NULL);
}

QWidget *QWidgetVideoSinkDelegate::widget() const
{
    return m_widget.data();
}

void QWidgetVideoSinkDelegate::setWidget(QWidget *widget)
{
    GST_LOG_OBJECT(m_sink, "Setting \"widget\" property to %" GST_PTR_FORMAT, widget);

    if (m_widget) {
        m_widget.data()->removeEventFilter(this);
        m_widget.data()->setAttribute(Qt::WA_OpaquePaintEvent, m_opaquePaintEventAttribute);
        m_widget.data()->update();

        m_widget = NULL;
    }

    if (widget) {
        widget->installEventFilter(this);
        m_opaquePaintEventAttribute = widget->testAttribute(Qt::WA_OpaquePaintEvent);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
        widget->update();

        m_widget = widget;
    }
}

bool QWidgetVideoSinkDelegate::eventFilter(QObject *filteredObject, QEvent *event)
{
    if (filteredObject == m_widget.data()) {
        switch(event->type()) {
        case QEvent::Paint:
          {
            QPainter painter(m_widget.data());
            paint(&painter, m_widget.data()->rect());
            return true;
          }
        default:
            return false;
        }
    } else {
        return QtVideoSinkDelegate::eventFilter(filteredObject, event);
    }
}

void QWidgetVideoSinkDelegate::update()
{
    if (m_widget) {
        m_widget.data()->update();
    }
}
