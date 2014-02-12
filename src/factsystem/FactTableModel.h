#ifndef FACTTABLEMODEL_H
#define FACTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include "FactHandler.h"
#include "FactTableWidget.h"

class UASInterface;

class FactTableModel :public QAbstractTableModel
{
    Q_OBJECT
    
public:
    FactTableModel(UASInterface* uas, FactTableWidget* driverTable, QObject* parent = NULL);
    
    // QAbstractItemModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const;
    
private slots:
    // Signals from ParameterMap
    void _factUpdated(int uasId, Fact::Provider_t provider, const QString& factId);
    
private:
    UASInterface*           _uas;
    FactHandler&            _factHandler;
    FactTableWidget*        _driverTable;
};

#endif