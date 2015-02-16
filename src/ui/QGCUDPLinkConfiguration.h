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
