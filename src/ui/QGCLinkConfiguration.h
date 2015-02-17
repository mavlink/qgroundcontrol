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
