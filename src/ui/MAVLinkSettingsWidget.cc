/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Implementation of MAVLinkSettingsWidget
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include "MAVLinkSettingsWidget.h"
#include "ui_MAVLinkSettingsWidget.h"

MAVLinkSettingsWidget::MAVLinkSettingsWidget(MAVLinkProtocol* protocol, QWidget *parent) :
    QWidget(parent),
    protocol(protocol),
    m_ui(new Ui::MAVLinkSettingsWidget)
{
    m_ui->setupUi(this);

    // Connect actions
    connect(protocol, SIGNAL(heartbeatChanged(bool)), m_ui->heartbeatCheckBox, SLOT(setChecked(bool)));
    connect(m_ui->heartbeatCheckBox, SIGNAL(toggled(bool)), protocol, SLOT(enableHeartbeats(bool)));
    connect(protocol, SIGNAL(loggingChanged(bool)), m_ui->loggingCheckBox, SLOT(setChecked(bool)));
    connect(m_ui->loggingCheckBox, SIGNAL(toggled(bool)), protocol, SLOT(enableLogging(bool)));

    // Initialize state
    m_ui->heartbeatCheckBox->setChecked(protocol->heartbeatsEnabled());
    m_ui->loggingCheckBox->setChecked(protocol->loggingEnabled());
}

MAVLinkSettingsWidget::~MAVLinkSettingsWidget()
{
    delete m_ui;
}

void MAVLinkSettingsWidget::changeEvent(QEvent *e)
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
