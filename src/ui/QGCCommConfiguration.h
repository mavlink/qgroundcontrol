/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Comm Link Configuration
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#ifndef QGCCOMMCONFIGURATION_H
#define QGCCOMMCONFIGURATION_H

#include <QWidget>
#include <QDialog>

#include "LinkConfiguration.h"

namespace Ui {
class QGCCommConfiguration;
}

class QGCCommConfiguration : public QDialog
{
    Q_OBJECT

public:
    explicit QGCCommConfiguration(QWidget *parent, LinkConfiguration* config = 0);
    ~QGCCommConfiguration();

    enum {
        QGC_LINK_SERIAL,
        QGC_LINK_UDP,
        QGC_LINK_TCP,
        QGC_LINK_SIMULATION,
        QGC_LINK_FORWARDING,
#ifdef QT_DEBUG
        QGC_LINK_MOCK,
#endif
#ifdef  QGC_XBEE_ENABLED
        QGC_LINK_XBEE,
#endif
#ifdef  QGC_RTLAB_ENABLED
        QGC_LINK_OPAL
#endif
    };

    LinkConfiguration* getConfig() { return _config; }

private slots:
    void on_typeCombo_currentIndexChanged(int index);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_nameEdit_textEdited(const QString &arg1);

private:
    void _changeLinkType(int type);
    void _loadTypeConfigWidget(int type);
    void _updateUI();

    Ui::QGCCommConfiguration* _ui;
    LinkConfiguration*        _config;
};

#endif // QGCCOMMCONFIGURATION_H
