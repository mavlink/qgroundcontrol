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
#include <QFileDialog>
#include <QDebug>
#include <QApplication>

#include "MG.h"
#include "UASListWidget.h"
#include "UASManager.h"
#include "UAS.h"
#include "UASView.h"
#include "QGCUnconnectedInfoWidget.h"
#include "MainWindow.h"
#include "MAVLinkSimulationLink.h"
#include "LinkManager.h"

UASListWidget::UASListWidget(QWidget *parent) : QWidget(parent),
    m_ui(new Ui::UASList)
{
    m_ui->setupUi(this);
    m_ui->verticalLayout->setAlignment(Qt::AlignTop);

    // Construct initial widget
    uWidget = new QGCUnconnectedInfoWidget(this);
    m_ui->verticalLayout->addWidget(uWidget);

    linkToBoxMapping = QMap<LinkInterface*, QGroupBox*>();
    uasViews = QMap<UASInterface*, UASView*>();

    this->setVisible(false);

    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
            this, SLOT(addUAS(UASInterface*)));

    // Get a list of all existing UAS
    foreach (UASInterface* uas, UASManager::instance()->getUASList()) {
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


void UASListWidget::addUAS(UASInterface* uas)
{
    if (uasViews.isEmpty())
    {
        m_ui->verticalLayout->removeWidget(uWidget);
        delete uWidget;
        uWidget = NULL;
    }

    if (!uasViews.contains(uas))
    {
        // Only display the UAS in a single link.
        QList<LinkInterface*>* x = uas->getLinks();
        if (x->size())
        {
            LinkInterface* li = x->at(0);

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
            }

            // And add the new UAS to the UASList
            UASView* newView = new UASView(uas, newBox);
            uasViews.insert(uas, newView);
            newBox->layout()->addWidget(newView);

            // Watch for when this widget is destroyed so that we can clean up the
            // groupbox if necessary.
            connect(newView, SIGNAL(destroyed(QObject*)),
                    this, SLOT(removeUASView(QObject*)));
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
    // Remove the UAS from our data structures and
    // the global listing.
    uasViews.remove(uas);

    // Check all groupboxes for all links this uas had and check if they're empty.
    // Delete them if they are.
    QListIterator<LinkInterface*> i = *uas->getLinks();
    while (i.hasNext())
    {
        LinkInterface* link = i.next();

        QGroupBox* box = linkToBoxMapping[link];
        if (box)
        {
            // If this was the last UAS in the GroupBox, remove it and its corresponding link.
            int views = box->findChildren<UASView*>().size();
            if (views == 0) {
                box->deleteLater();
                linkToBoxMapping.remove(link);
            }
        }
    }

    // And if no QGroupBoxes are left, put the initial widget back.
    uWidget = new QGCUnconnectedInfoWidget(this);
    m_ui->verticalLayout->addWidget(uWidget);
}
