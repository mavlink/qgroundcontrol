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
 *   @brief Implementation of QGCLinkConfiguration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef QGCLINKCONFIGURATION_H
#define QGCLINKCONFIGURATION_H

#include <QWidget>
#include <QListView>

#include "LinkManager.h"

namespace Ui {
class QGCLinkConfiguration;
}

class LinkViewModel;

class QGCLinkConfiguration : public QWidget
{
    Q_OBJECT

public:
    explicit QGCLinkConfiguration(QWidget *parent = 0);
    ~QGCLinkConfiguration();

private slots:
    void on_delLinkButton_clicked();
    void on_editLinkButton_clicked();
    void on_addLinkButton_clicked();
    void on_linkView_doubleClicked(const QModelIndex &index);
    void on_linkView_clicked(const QModelIndex &index);
    void on_connectLinkButton_clicked();

private:
    void _editLink(int row);
    void _fixUnnamed(LinkConfiguration* config);
    void _updateButtons();

    Ui::QGCLinkConfiguration* _ui;
    LinkViewModel*            _viewModel;
};

class LinkViewModel : public QAbstractListModel
{
public:
    LinkViewModel(QObject *parent = 0);
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    LinkConfiguration* getConfiguration(int row);
    void beginChange() { beginResetModel(); }
    void endChange() { endResetModel(); }
};

#endif // QGCLINKCONFIGURATION_H
