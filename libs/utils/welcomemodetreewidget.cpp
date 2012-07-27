/**
 ******************************************************************************
 *
 * @file       welcomemodetreewidget.cpp
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

#include "welcomemodetreewidget.h"

#include <QtGui/QLabel>
#include <QtGui/QAction>
#include <QtGui/QBoxLayout>
#include <QtGui/QHeaderView>

namespace Utils {

void WelcomeModeLabel::setStyledText(const QString &text)
{
    QString  rc = QLatin1String(
    "<html><head><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head>"
    "<body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">"
    "<p style=\" margin-top:16px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
    "<span style=\" font-size:x-large; color:#555555;\">");
    rc += text;
    rc += QLatin1String("</span></p><hr/></body></html>");
    setText(rc);
}

struct WelcomeModeTreeWidgetPrivate
{
    WelcomeModeTreeWidgetPrivate() {}
    QIcon bullet;
};

WelcomeModeTreeWidget::WelcomeModeTreeWidget(QWidget *parent) :
        QTreeWidget(parent), m_d(new WelcomeModeTreeWidgetPrivate)
{
    m_d->bullet = QIcon(QLatin1String(":/welcome/images/list_bullet_arrow.png"));
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            SLOT(slotItemClicked(QTreeWidgetItem *)));

    viewport()->setAutoFillBackground(false);
}

WelcomeModeTreeWidget::~WelcomeModeTreeWidget()
{
    delete m_d;
}

QSize WelcomeModeTreeWidget::minimumSizeHint() const
{
    return QSize();
}

QSize WelcomeModeTreeWidget::sizeHint() const
{
    return QSize(QTreeWidget::sizeHint().width(), 30 * topLevelItemCount());
}

QTreeWidgetItem *WelcomeModeTreeWidget::addItem(const QString &label, const QString &data, const QString &toolTip)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(this);
    item->setIcon(0, m_d->bullet);
    item->setSizeHint(0, QSize(24, 30));
    QLabel *lbl = new QLabel(label);
    lbl->setTextInteractionFlags(Qt::NoTextInteraction);
    lbl->setCursor(QCursor(Qt::PointingHandCursor));
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QBoxLayout *lay = new QVBoxLayout;
    lay->setContentsMargins(3, 2, 0, 0);
    lay->addWidget(lbl);
    QWidget *wdg = new QWidget;
    wdg->setLayout(lay);
    setItemWidget(item, 1, wdg);
    item->setData(0, Qt::UserRole, data);
    if (!toolTip.isEmpty())
        wdg->setToolTip(toolTip);
    return item;

}

void WelcomeModeTreeWidget::slotAddNewsItem(const QString &title, const QString &description, const QString &link)
{
    int itemWidth = width()-header()->sectionSize(0);
    QFont f = font();
    QString elidedText = QFontMetrics(f).elidedText(description, Qt::ElideRight, itemWidth);
    f.setBold(true);
    QString elidedTitle = QFontMetrics(f).elidedText(title, Qt::ElideRight, itemWidth);
    QString data = QString::fromLatin1("<b>%1</b><br />%2").arg(elidedTitle).arg(elidedText);
    addTopLevelItem(addItem(data, link, link));
}

void WelcomeModeTreeWidget::slotItemClicked(QTreeWidgetItem *item)
{
    emit activated(item->data(0, Qt::UserRole).toString());
}

}
