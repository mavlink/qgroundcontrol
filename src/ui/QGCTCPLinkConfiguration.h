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

#ifndef QGCTCPLINKCONFIGURATION_H
#define QGCTCPLINKCONFIGURATION_H

#include <QWidget>

#include "TCPLink.h"

namespace Ui
{
class QGCTCPLinkConfiguration;
}

class QGCTCPLinkConfiguration : public QWidget
{
    Q_OBJECT
public:
    explicit QGCTCPLinkConfiguration(TCPConfiguration *config, QWidget *parent = 0);
    ~QGCTCPLinkConfiguration();

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_portSpinBox_valueChanged(int arg1);
    void on_hostAddressLineEdit_textChanged(const QString &arg1);

private:
    Ui::QGCTCPLinkConfiguration* _ui;
    TCPConfiguration*            _config;
};

#endif // QGCTCPLINKCONFIGURATION_H
