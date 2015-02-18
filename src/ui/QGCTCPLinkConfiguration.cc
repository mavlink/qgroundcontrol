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

/**
 * @file
 *   @brief Implementation of QGCTCPLinkConfiguration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include <QInputDialog>

#include "QGCTCPLinkConfiguration.h"
#include "ui_QGCTCPLinkConfiguration.h"

QGCTCPLinkConfiguration::QGCTCPLinkConfiguration(TCPConfiguration *config, QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::QGCTCPLinkConfiguration)
    , _config(config)
{
    Q_ASSERT(_config != NULL);
    _ui->setupUi(this);
    quint16 port = config->port();
    _ui->portSpinBox->setValue(port);
    QString addr = config->address().toString();
    _ui->hostAddressLineEdit->setText(addr);
}

QGCTCPLinkConfiguration::~QGCTCPLinkConfiguration()
{
    delete _ui;
}

void QGCTCPLinkConfiguration::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        _ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void QGCTCPLinkConfiguration::on_portSpinBox_valueChanged(int arg1)
{
   _config->setPort(arg1);
}

void QGCTCPLinkConfiguration::on_hostAddressLineEdit_textChanged(const QString &arg1)
{
    QHostAddress add(arg1);
    _config->setAddress(add);
}
