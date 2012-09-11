/**
 ******************************************************************************
 *
 * @file       detailswidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief      
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   
 * @{
 * 
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "detailswidget.h"
#include "detailsbutton.h"

#include <QtGui/QGridLayout>
#include <QtCore/QStack>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>
#include <QtGui/QPainter>

using namespace Utils;

DetailsWidget::DetailsWidget(QWidget *parent)
    : QWidget(parent),
      m_summaryLabel(new QLabel(this)),
      m_detailsButton(new DetailsButton(this)),
      m_widget(0),
      m_toolWidget(0),
      m_grid(new QGridLayout(this))

{
    m_grid->setContentsMargins(4, 3, 4, 3);

    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_summaryLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_grid->addWidget(m_summaryLabel, 0, 0);
    m_grid->addWidget(m_detailsButton, 0, 2, 1, 1, Qt::AlignBottom);

    m_dummyWidget = new QWidget(this);
    m_dummyWidget->setMaximumHeight(4);
    m_dummyWidget->setMaximumHeight(4);
    m_dummyWidget->setVisible(false);
    m_grid->addWidget(m_dummyWidget, 2, 0, 1, 1);

    connect(m_detailsButton, SIGNAL(clicked()),
            this, SLOT(detailsButtonClicked()));
}

DetailsWidget::~DetailsWidget()
{

}

void DetailsWidget::paintEvent(QPaintEvent *paintEvent)
{
    //TL-->                 ___________  <-- TR
    //                     |           |
    //ML->   ______________| <--MM     | <--MR
    //       |                         |
    //BL->   |_________________________| <-- BR


    QWidget::paintEvent(paintEvent);

    if (!m_detailsButton->isToggled())
        return;

    const QRect detailsGeometry = m_detailsButton->geometry();
    const QRect widgetGeometry = m_widget ? m_widget->geometry() : QRect(x(), y() + height(), width(), 0);

    QPoint tl(detailsGeometry.topLeft());
    tl += QPoint(-3, -3);

    QPoint tr(detailsGeometry.topRight());
    tr += QPoint(3, -3);

    QPoint mm(detailsGeometry.left() - 3, widgetGeometry.top() - 3);

    QPoint ml(1, mm.y());

    QPoint mr(tr.x(), mm.y());

    int bottom = geometry().height() - 3;
    QPoint bl(1, bottom);
    QPoint br(tr.x(), bottom);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    p.setBrush(palette().dark());
    p.drawRoundedRect(QRect(tl, br), 5, 5);
    p.drawRoundedRect(QRect(ml, br), 5, 5);
}

void DetailsWidget::detailsButtonClicked()
{
    bool visible = m_detailsButton->isToggled();
    if (m_widget)
        m_widget->setVisible(visible);
    m_dummyWidget->setVisible(visible);
    fixUpLayout();
}

void DetailsWidget::setSummaryText(const QString &text)
{
    m_summaryLabel->setText(text);
}

QString DetailsWidget::summaryText() const
{
    return m_summaryLabel->text();
}

bool DetailsWidget::expanded() const
{
    return m_detailsButton->isToggled();
}

void DetailsWidget::setExpanded(bool v)
{
    if (expanded() != v)
        m_detailsButton->animateClick();
}

QWidget *DetailsWidget::widget() const
{
    return m_widget;
}

void DetailsWidget::setWidget(QWidget *widget)
{
    if (m_widget == widget)
        return;
    if (m_widget) {
        m_grid->removeWidget(m_widget);
        m_widget = 0;
    }
    if (widget) {
        m_grid->addWidget(widget, 1, 0, 1, 3);
        m_widget = widget;
        bool visible = m_detailsButton->isToggled();
        m_widget->setVisible(visible);
        m_dummyWidget->setVisible(visible);
    }
}

void DetailsWidget::setToolWidget(QWidget *widget)
{
    if (m_toolWidget == widget)
        return;
    if (m_toolWidget) {
        m_grid->removeWidget(m_toolWidget);
        m_toolWidget = 0;
    }
    if (widget) {
        m_grid->addWidget(widget, 0, 1, 1, 1, Qt::AlignBottom);
        m_toolWidget = widget;
    }
}

QWidget *DetailsWidget::toolWidget() const
{
    return m_toolWidget;
}

void DetailsWidget::fixUpLayout()
{
    if (!m_widget)
        return;
    QWidget *parent = m_widget;
    QStack<QWidget *> widgets;
    while((parent = parent->parentWidget()) && parent && parent->layout()) {
        widgets.push(parent);
        parent->layout()->update();
    }

    while(!widgets.isEmpty()) {
        widgets.pop()->layout()->activate();
    }
}
