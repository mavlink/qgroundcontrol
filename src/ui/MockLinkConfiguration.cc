/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "MockLinkConfiguration.h"
#include "ui_MockLinkConfiguration.h"

MockLinkConfiguration::MockLinkConfiguration(MockConfiguration *config, QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::MockLinkConfiguration)
    , _config(config)
{
    _ui->setupUi(this);

    switch (config->firmwareType()) {
        case MAV_AUTOPILOT_PX4:
            _ui->px4Radio->setChecked(true);
            break;
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            _ui->apmRadio->setChecked(true);
            break;
        default:
            _ui->genericRadio->setChecked(true);
            break;
    }

    _ui->sendStatusTextCheckBox->setChecked(config->sendStatusText());

    connect(_ui->px4Radio,                  &QRadioButton::clicked, this, &MockLinkConfiguration::_px4RadioClicked);
    connect(_ui->apmRadio,                  &QRadioButton::clicked, this, &MockLinkConfiguration::_apmRadioClicked);
    connect(_ui->genericRadio,              &QRadioButton::clicked, this, &MockLinkConfiguration::_genericRadioClicked);
    connect(_ui->sendStatusTextCheckBox,    &QCheckBox::clicked,    this, &MockLinkConfiguration::_sendStatusTextClicked);
}

MockLinkConfiguration::~MockLinkConfiguration()
{
    delete _ui;
}

void MockLinkConfiguration::_px4RadioClicked(bool checked)
{
    if (checked) {
        _config->setFirmwareType(MAV_AUTOPILOT_PX4);
    }
}

void MockLinkConfiguration::_apmRadioClicked(bool checked)
{
    if (checked) {
        _config->setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    }
}

void MockLinkConfiguration::_genericRadioClicked(bool checked)
{
    if (checked) {
        _config->setFirmwareType(MAV_AUTOPILOT_GENERIC);
    }
}

void MockLinkConfiguration::_sendStatusTextClicked(bool checked)
{
    _config->setSendStatusText(checked);
}
