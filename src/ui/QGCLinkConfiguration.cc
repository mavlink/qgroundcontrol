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

#include "SettingsDialog.h"
#include "QGCLinkConfiguration.h"
#include "ui_QGCLinkConfiguration.h"
#include "QGCCommConfiguration.h"
#include "QGCMessageBox.h"

QGCLinkConfiguration::QGCLinkConfiguration(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::QGCLinkConfiguration)
{
    // Stop automatic link updates while this UI is up
    LinkManager::instance()->suspendConfigurationUpdates(true);
    _ui->setupUi(this);
    _viewModel = new LinkViewModel;
    _ui->linkView->setModel(_viewModel);
    _ui->connectLinkButton->setEnabled(false);
    _ui->delLinkButton->setEnabled(false);
    _ui->editLinkButton->setEnabled(false);
}

QGCLinkConfiguration::~QGCLinkConfiguration()
{
    if(_viewModel) delete _viewModel;
    if(_ui) delete _ui;
    // Resume automatic link updates
    LinkManager::instance()->suspendConfigurationUpdates(false);
}

void QGCLinkConfiguration::on_delLinkButton_clicked()
{
    QModelIndex index = _ui->linkView->currentIndex();
    LinkConfiguration* config = _viewModel->getConfiguration(index.row());
    if(config) {
        // Ask user if they are sure
        QMessageBox::StandardButton button = QGCMessageBox::question(
            tr("Delete Link Configuration"),
            tr("Are you sure you want to delete %1?\nDeleting a configuration will also disconnect it if connected.").arg(config->name()),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (button == QMessageBox::Yes) {
            // Get link attached to this configuration (if any)
            LinkInterface* iface = config->getLink();
            if(iface) {
                // Disconnect it (if connected)
                LinkManager::instance()->disconnectLink(iface);
            }
            _viewModel->beginChange();
            // Remove configuration
            LinkManager::instance()->removeLinkConfiguration(config);
            // Save list
            LinkManager::instance()->saveLinkConfigurationList();
            _viewModel->endChange();
        }
    }
}

void QGCLinkConfiguration::on_linkView_clicked(const QModelIndex &index)
{
    LinkConfiguration* config = _viewModel->getConfiguration(index.row());
    bool enabled = (config && !config->getLink());
    _ui->connectLinkButton->setEnabled(enabled);
    _ui->delLinkButton->setEnabled(config != NULL);
    _ui->editLinkButton->setEnabled(config != NULL);
}

void QGCLinkConfiguration::on_connectLinkButton_clicked()
{
    QModelIndex index = _ui->linkView->currentIndex();
    LinkConfiguration* config = _viewModel->getConfiguration(index.row());
    if(config) {
        // Only connect if not already connected
        if(!config->getLink()) {
            LinkInterface* link = LinkManager::instance()->createLink(config);
            if(link) {
                // Connect it
                LinkManager::instance()->connectLink(link);
                // Now go hunting for the parent so we can shut this down
                QWidget* pQw = parentWidget();
                while(pQw) {
                    SettingsDialog* pDlg = dynamic_cast<SettingsDialog*>(pQw);
                    if(pDlg) {
                        pDlg->accept();
                        break;
                    }
                    pQw = pQw->parentWidget();
                }
            }
        }
    }
}

void QGCLinkConfiguration::on_editLinkButton_clicked()
{
    QModelIndex index = _ui->linkView->currentIndex();
    _editLink(index.row());
}

void QGCLinkConfiguration::_fixUnnamed(LinkConfiguration* config)
{
    Q_ASSERT(config != NULL);
    //-- Check for "Unnamed"
    if (config->name() == tr("Unnamed")) {
        switch(config->type()) {
            case LinkConfiguration::TypeSerial:
                config->setName(
                    QString("Serial Device on %1").arg(dynamic_cast<SerialConfiguration*>(config)->portName()));
                break;
            case LinkConfiguration::TypeUdp:
                config->setName(
                    QString("UDP Link on Port %1").arg(dynamic_cast<UDPConfiguration*>(config)->localPort()));
                break;
            case LinkConfiguration::TypeTcp: {
                    TCPConfiguration* tconfig = dynamic_cast<TCPConfiguration*>(config);
                    if(tconfig) {
                        config->setName(
                            QString("TCP Link %1:%2").arg(tconfig->address().toString()).arg((int)tconfig->port()));
                    }
                }
                break;
#ifdef UNITTEST_BUILD
            case LinkConfiguration::TypeMock:
                config->setName(
                    QString("Mock Link"));
                break;
#endif
        }
    }
}

void QGCLinkConfiguration::on_addLinkButton_clicked()
{
    QGCCommConfiguration* commDialog = new QGCCommConfiguration(this);
    if(commDialog->exec() == QDialog::Accepted) {
        // Save changes (if any)
        LinkConfiguration* config = commDialog->getConfig();
        if(config) {
            _fixUnnamed(config);
            _viewModel->beginChange();
            LinkManager::instance()->addLinkConfiguration(commDialog->getConfig());
            LinkManager::instance()->saveLinkConfigurationList();
            _viewModel->endChange();
        }
    }
}

void QGCLinkConfiguration::on_linkView_doubleClicked(const QModelIndex &index)
{
    _editLink(index.row());
}

void QGCLinkConfiguration::_editLink(int row)
{
    LinkConfiguration* config = _viewModel->getConfiguration(row);
    if(config) {
        LinkConfiguration* tmpConfig = LinkConfiguration::duplicateSettings(config);
        QGCCommConfiguration* commDialog = new QGCCommConfiguration(this, tmpConfig);
        if(commDialog->exec() == QDialog::Accepted) {
            // Save changes (if any)
            if(commDialog->getConfig()) {
                _fixUnnamed(tmpConfig);
                _viewModel->beginChange();
                config->copyFrom(tmpConfig);
                // Save it
                LinkManager::instance()->saveLinkConfigurationList();
                _viewModel->endChange();
                // Tell link about changes (if any)
                config->updateSettings();
            }
        }
        // Discard temporary duplicate
        if(commDialog->getConfig())
            delete commDialog->getConfig();
    }
}

LinkViewModel::LinkViewModel(QObject *parent) : QAbstractListModel(parent)
{
    Q_UNUSED(parent);
}

int LinkViewModel::rowCount( const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    QList<LinkConfiguration*> cfgList = LinkManager::instance()->getLinkConfigurationList();
    int count = cfgList.count();
    return count;
}

QVariant LinkViewModel::data( const QModelIndex & index, int role) const
{
    QList<LinkConfiguration*> cfgList = LinkManager::instance()->getLinkConfigurationList();
    if (role == Qt::DisplayRole && index.row() < cfgList.count()) {
        QString name(cfgList.at(index.row())->name());
        return name;
    }
    return QVariant();
}

LinkConfiguration* LinkViewModel::getConfiguration(int row)
{
    QList<LinkConfiguration*> cfgList = LinkManager::instance()->getLinkConfigurationList();
    if(row < cfgList.count()) {
        return cfgList.at(row);
    }
    return NULL;
}

