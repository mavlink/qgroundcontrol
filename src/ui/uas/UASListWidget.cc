/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief List of unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QString>
#include <QTimer>
#include <QLabel>
#include <QDebug>
#include <QApplication>

#include "MG.h"
#include "UASListWidget.h"
#include "UASManager.h"
#include "UAS.h"
#include "UASView.h"
#include "QGCUnconnectedInfoWidget.h"
#include "MainWindow.h"
#include "LinkManager.h"

UASListWidget::UASListWidget(QWidget *parent) : QWidget(parent),
    uWidget(NULL),
    m_ui(new Ui::UASList)
{
    // Use a timer to update the link health display.
    updateTimer = new QTimer(this);
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateStatus()));

    m_ui->setupUi(this);
    m_ui->verticalLayout->setAlignment(Qt::AlignTop);

    this->setMinimumWidth(262);

    linkToBoxMapping = QMap<LinkInterface*, QGroupBox*>();
    uasToBoxMapping = QMap<UASInterface*, QGroupBox*>();
    uasViews = QMap<UASInterface*, UASView*>();

    this->setVisible(false);

    connect(LinkManager::instance(), SIGNAL(linkDeleted(LinkInterface*)), this, SLOT(removeLink(LinkInterface*)));

    // Listen for when UASes are added or removed. This does not manage the UASView
    // widgets that are displayed within this widget.
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
            this, SLOT(addUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(UASDeleted(UASInterface*)),
            this, SLOT(removeUAS(UASInterface*)));

    // Get a list of all existing UAS
    foreach (UASInterface* uas, UASManager::instance()->getUASList())
    {
        addUAS(uas);
    }
}

UASListWidget::~UASListWidget()
{
    delete m_ui;
}

void UASListWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

// XXX This is just to prevent
// upfront crashes, will probably need further inspection
void UASListWidget::removeLink(LinkInterface* link)
{
    QGroupBox* box = linkToBoxMapping.value(link, NULL);

    if (box) {
        // Just stop updating the status for now - we should
        // remove the UAS probably
        linkToBoxMapping.remove(link);
    }
}

void UASListWidget::updateStatus()
{
    QMapIterator<LinkInterface*, QGroupBox*> i(linkToBoxMapping);
    while (i.hasNext()) {
        i.next();
        LinkInterface* link = i.key();

        // Paranoid sanity check
        if (!LinkManager::instance()->containsLink(link))
            continue;

        if (!link)
            continue;

        MAVLinkProtocol* mavlink = MAVLinkProtocol::instance();

        // Build the tooltip out of the protocol parsing data: received, dropped, and parsing errors.
        QString displayString("");
        int c;
        if ((c = mavlink->getReceivedPacketCount(link)) != -1)
        {
            displayString += QString(tr("<br/>Received: %2")).arg(QString::number(c));
        }
        if ((c = mavlink->getDroppedPacketCount(link)) != -1)
        {
            displayString += QString(tr("<br/>Dropped: %2")).arg(QString::number(c));
        }
        if ((c = mavlink->getParsingErrorCount(link)) != -1)
        {
            displayString += QString(tr("<br/>Errors: %2")).arg(QString::number(c));
        }
        if (!displayString.isEmpty())
        {
            displayString = QString("<b>%1</b>").arg(i.key()->getName()) + displayString;
        }
//        qDebug() << p << ": " + displayString;
        i.value()->setToolTip(displayString);
    }
}

void UASListWidget::addUAS(UASInterface* uas)
{
    // If the list was empty, remove the unconnected widget and start the update timer.
    if (uasViews.isEmpty())
    {
        updateTimer->start(5000);

        if (uWidget)
        {
            m_ui->verticalLayout->removeWidget(uWidget);
            delete uWidget;
            uWidget = NULL;
        }
    }
    if (!uasViews.contains(uas))
    {
        // Only display the UAS in a single link.
        QList<LinkInterface*> x = uas->getLinks();
        if (x.size())
        {
            LinkInterface* li = x.first();

            // Find an existing QGroupBox for this LinkInterface or create a
            // new one.
            QGroupBox* newBox;
            if (linkToBoxMapping.contains(li))
            {
                newBox = linkToBoxMapping[li];
            }
            else
            {
                newBox = new QGroupBox(li->getName(), this);
                QVBoxLayout* boxLayout = new QVBoxLayout(newBox);
                newBox->setLayout(boxLayout);
                m_ui->verticalLayout->addWidget(newBox);
                linkToBoxMapping[li] = newBox;
                updateStatus(); // Update the link status for this GroupBox.
            }

            // And add the new UAS to the UASList
            UASView* newView = new UASView(uas, newBox);
            uasViews.insert(uas, newView);
            uasToBoxMapping[uas] = newBox;
            newBox->layout()->addWidget(newView);
        }
    }
}

void UASListWidget::activeUAS(UASInterface* uas)
{
    UASView* view = uasViews.value(uas, NULL);
    if (view) {
        view->setUASasActive(true);
    }
}

/**
 * If the UAS was removed, check to see if it was the last one in the QGroupBox and delete
 * the QGroupBox if so.
 */
void UASListWidget::removeUAS(UASInterface* uas)
{
    // Remove the UASView and check if its parent GroupBox has any other children,
    // delete it if it doesn't.
    QGroupBox* box = uasToBoxMapping[uas];
    uasToBoxMapping.remove(uas);
    uasViews.remove(uas);
    int otherViews = 0;
    foreach (UASView* view, box->findChildren<UASView*>())
    {
        if (view->uas == uas)
        {
            view->deleteLater();
        }
        else
        {
            ++otherViews;
        }
    }
    if (otherViews == 0)
    {
        // Delete the groupbox.
        QMap<LinkInterface*, QGroupBox*>::const_iterator i = linkToBoxMapping.constBegin();
        while (i != linkToBoxMapping.constEnd()) {
            if (i.value() == box)
            {
                linkToBoxMapping.remove(i.key());
                break;
            }
            ++i;
        }
        box->deleteLater();

        // And if no other QGroupBoxes are left, put the initial widget back.
        // We also stop the update timer as there's nothing to update at this point.
        int otherBoxes = 0;
        foreach (const QGroupBox* otherBox, findChildren<QGroupBox*>())
        {
            if (otherBox != box)
            {
                ++otherBoxes;
            }
        }
        if (otherBoxes == 0)
        {
            uWidget = new QGCUnconnectedInfoWidget(this);
            m_ui->verticalLayout->addWidget(uWidget);
            updateTimer->stop();
        }
    }
}
