/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "SummaryPage.h"
#include "ui_SummaryPage.h"

#include <QGroupBox>
#include <QTableWidget>
#include <QSpacerItem>
#include <QDebug>

SummaryPage::SummaryPage(QList<VehicleComponent*>& components, QWidget* parent) :
    QWidget(parent),
    _components(components),
    _ui(new Ui::SummaryPage)
{
    _ui->setupUi(this);
    
    // Loop over all components, creating summary widgets
    
    int row = 0;
    int col = 0;
    foreach(VehicleComponent* component, _components) {
        QList<QStringList> summaryItems = component->summaryItems();
        
        if (summaryItems.count()) {
            // Rows without columns, bad
            Q_ASSERT(summaryItems[0].count());
            
            QGroupBox* box = new QGroupBox(component->name(), this);
            QVBoxLayout* layout = new QVBoxLayout(box);
            
            QTableWidget* table = new QTableWidget(summaryItems.count(), summaryItems[0].count(), box);
            table->setShowGrid(false);
            table->horizontalHeader()->setVisible(false);
            table->verticalHeader()->setVisible(false);
            
            QTableWidgetItem* item;
            
            for (int tRow=0; tRow<summaryItems.count(); tRow++) {
                for (int tCol=0; tCol<summaryItems[tRow].count(); tCol++) {
                    item = new QTableWidgetItem(summaryItems[tRow][tCol]);
                    table->setItem(tRow, tCol, item);
                }
            }
            table->resizeColumnsToContents();
            
            layout->addWidget(table);

            _ui->gridLayout->addWidget(box, row, col++, Qt::AlignLeft | Qt::AlignTop);
        }
        
        if (col == 2) {
            row++;
            col = 0;
        }
    }
    
    // Add spacers to force the grid to collapse to it's smallest size
    QSpacerItem* spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    _ui->gridLayout->addItem(spacer, 0, 2, 1, 1);
    spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    _ui->gridLayout->addItem(spacer, row, 0, 1, 1);
}

SummaryPage::~SummaryPage()
{
    delete _ui;
}
