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
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QHBoxLayout>
#include <LinechartContainer.h>

LinechartContainer::LinechartContainer(QWidget* parent) : QWidget(parent)
{
//    setMinimumHeight(300);
//    setMinimumWidth(450);
    setContentsMargins(0, 0, 0, 0); // No margin around container content
}

LinechartContainer::~LinechartContainer()
{

}

LinechartPlot* LinechartContainer::getPlot()
{
    return plot;
}

void LinechartContainer::setPlot(LinechartPlot* plot)
{
    this->plot = plot;
    delete layout();
    QHBoxLayout* currentLayout = new QHBoxLayout(this);
    currentLayout->addWidget(plot);
    setLayout(currentLayout);
}
