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

#include <QInputDialog>

#include "QGCUDPLinkConfiguration.h"
#include "ui_QGCUDPLinkConfiguration.h"

QGCUDPLinkConfiguration::QGCUDPLinkConfiguration(UDPConfiguration *config, QWidget *parent)
    : QWidget(parent)
    , _inConstructor(true)
    , _ui(new Ui::QGCUDPLinkConfiguration)
{
    _config = config;
    _ui->setupUi(this);
    _viewModel = new UPDViewModel;
    _ui->listView->setModel(_viewModel);
    _ui->removeHost->setEnabled(false);
    _ui->editHost->setEnabled(false);
    _ui->portNumber->setRange(1024, 65535);
    _ui->portNumber->setValue(_config->localPort());
    _reloadList();
    _inConstructor = false;
}

QGCUDPLinkConfiguration::~QGCUDPLinkConfiguration()
{
    delete _ui;
}

void QGCUDPLinkConfiguration::_reloadList()
{
    QString host;
    int port;
    if(_config->firstHost(host, port)) {
        _viewModel->beginChange();
        _viewModel->hosts.clear();
        do {
            _viewModel->hosts.append(QString("%1:%2").arg(host, QString::number(port)));
        } while (_config->nextHost(host, port));
        _viewModel->endChange();
    }
}

void QGCUDPLinkConfiguration::_editHost(int row)
{
    if(row < _viewModel->hosts.count()) {
        bool ok;
        QString oldName = _viewModel->hosts.at(row);
        QString hostName = QInputDialog::getText(
            this, tr("Edit a MAVLink host target"),
            tr("Host (hostname:port):                                                     "), QLineEdit::Normal, oldName, &ok);
        if (ok && !hostName.isEmpty()) {
            _viewModel->beginChange();
            _viewModel->hosts.replace(row, hostName);
            _viewModel->endChange();
            _config->removeHost(oldName);
            _config->addHost(hostName);
        }
    }
}

void QGCUDPLinkConfiguration::on_portNumber_valueChanged(int arg1)
{
    if(!_inConstructor) {
        _config->setLocalPort(arg1);
        _config->setDynamic(false);
    }
}

void QGCUDPLinkConfiguration::on_listView_clicked(const QModelIndex &index)
{
    bool enabled = index.row() < _viewModel->hosts.count();
    _ui->removeHost->setEnabled(enabled);
    _ui->editHost->setEnabled(enabled);
}

void QGCUDPLinkConfiguration::on_listView_doubleClicked(const QModelIndex &index)
{
    _editHost(index.row());
}

void QGCUDPLinkConfiguration::on_addHost_clicked()
{
    bool ok;
    QString hostName = QInputDialog::getText(
        this, tr("Add a host target to MAVLink"),
        tr("Host (hostname:port):                                                     "),
        QLineEdit::Normal, QString("localhost:%1").arg(QGC_UDP_TARGET_PORT), &ok);
    if (ok && !hostName.isEmpty()) {
        _config->addHost(hostName);
        _reloadList();
    }
}

void QGCUDPLinkConfiguration::on_removeHost_clicked()
{
    QModelIndex index = _ui->listView->currentIndex();
    if(index.row() < _viewModel->hosts.count()) {
        QString oldName = _viewModel->hosts.at(index.row());
        _viewModel->hosts.removeAt(index.row());
        _config->removeHost(oldName);
        _reloadList();
    }
}

void QGCUDPLinkConfiguration::on_editHost_clicked()
{
    QModelIndex index = _ui->listView->currentIndex();
    _editHost(index.row());
}


UPDViewModel::UPDViewModel(QObject *parent) : QAbstractListModel(parent)
{
    Q_UNUSED(parent);
}

int UPDViewModel::rowCount( const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return hosts.count();
}

QVariant UPDViewModel::data( const QModelIndex & index, int role) const
{
    if (role == Qt::DisplayRole && index.row() < hosts.count()) {
        return hosts.at(index.row());
    }
    return QVariant();
}
