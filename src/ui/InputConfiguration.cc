/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <InputConfigurationWidget.h>
#include <MG.h>

#include <QDebug>

InputConfigurationWidget::InputConfigurationWidget(QWidget *parent) : QWidget(parent)
{
    /* Add UI components */
    ui.setupUi(this);
}

InputConfigurationWidget::~InputConfigurationWidget()
{

}
