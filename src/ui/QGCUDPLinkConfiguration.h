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
 *   @brief Implementation of QGCUDPLinkConfiguration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef QGCUDPLINKCONFIGURATION_H
#define QGCUDPLINKCONFIGURATION_H

#include <QWidget>
#include <QListView>

#include "UDPLink.h"

namespace Ui {
class QGCUDPLinkConfiguration;
}

class UPDViewModel;

class QGCUDPLinkConfiguration : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUDPLinkConfiguration(UDPConfiguration* config, QWidget *parent = 0);
    ~QGCUDPLinkConfiguration();

private slots:
    void on_addHost_clicked();
    void on_removeHost_clicked();
    void on_editHost_clicked();
    void on_listView_clicked(const QModelIndex &index);
    void on_listView_doubleClicked(const QModelIndex &index);

    void on_portNumber_valueChanged(int arg1);

private:

    void _reloadList();
    void _editHost(int row);

    bool _inConstructor;
    Ui::QGCUDPLinkConfiguration* _ui;
    UDPConfiguration*            _config;
    UPDViewModel*                _viewModel;
};

class UPDViewModel : public QAbstractListModel
{
public:
    UPDViewModel(QObject *parent = 0);
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    void beginChange() { beginResetModel(); }
    void endChange() { endResetModel(); }
    QStringList hosts;
};

#endif // QGCUDPLINKCONFIGURATION_H
