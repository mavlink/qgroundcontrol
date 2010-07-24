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

#include "MG.h"
#include "UASListWidget.h"
#include "UASManager.h"
#include "UAS.h"
#include "UASView.h"

UASListWidget::UASListWidget(QWidget *parent) : QWidget(parent), m_ui(new Ui::UASList)
{
    m_ui->setupUi(this);

    listLayout = new QVBoxLayout(this);
    listLayout->setAlignment(Qt::AlignTop);
    this->setLayout(listLayout);

    this->setMinimumWidth(250);

    uasViews = QMap<UASInterface*, UASView*>();

    this->setVisible(false);
}

UASListWidget::~UASListWidget()
{
    delete m_ui;
}

void UASListWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void UASListWidget::addUAS(UASInterface* uas)
{
    if (uasViews.size() == 0)
    {
        // TODO Add label when no UAS is present yet.
    }

    if (!uasViews.contains(uas))
    {
        uasViews.insert(uas, new UASView(uas, this));
        listLayout->addWidget(uasViews.value(uas));
        //connect(uas, SIGNAL(destroyed(QObject*)), this, SLOT(removeUAS(QObject*)));
    }
}

void UASListWidget::activeUAS(UASInterface* uas)
{
    UASView* view = uasViews.value(uas, NULL);
    if (view)
    {
        view->setUASasActive(true);
    }
}

void UASListWidget::removeUAS(UASInterface* uas)
{
    listLayout->removeWidget(uasViews.value(uas));
    uasViews.value(uas)->deleteLater();
}

