#include "FactTableModel.h"
#include "FactSystem.h"
#include "FactTableWidget.h"
#include "UASInterface.h"
#include <QDebug>
#include <QFont>
#include <QColor>
#include <QBrush>
#include <QMetaObject>
#include <QMetaProperty>

/// Used to provide a Model in MVC terminology for the fact QTableView
///     @param[in] driverTable Table widget which contains the data which is used to driver to fact table view
FactTableModel::FactTableModel(UASInterface* uas, FactTableWidget* driverTable, QObject* parent) :
    QAbstractTableModel(parent),
    _uas(uas),
    _factHandler(uas->getFactHandler()),
    _driverTable(driverTable)
{
    Q_ASSERT(driverTable);
    
    // Connect to the fact signals to receive updates and adds. This allow us to show live data at all times.
    
    bool connected = connect(&_factHandler,
                             SIGNAL(factUpdated(int, Fact::Provider_t, const QString&)),
                             this,
                             SLOT(_factUpdated(int, Fact::Provider_t, const QString&)));
    Q_ASSERT_X(connected, "ParameterMap::factUpdated", "connect failed");
}

int FactTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return _driverTable->rowCount();
}

int FactTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return _driverTable->columnCount();
}

QVariant FactTableModel::data(const QModelIndex & index, int role) const
{
    QVariant retVariant;
    
    //qDebug() << "data" << Qt::EditRole << role;
    
    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::BackgroundRole || role == Qt::ForegroundRole || role == Qt::ToolTipRole) {
        bool    factFound;
        QString property;
        QString cellValue;
        
        Fact& fact = _driverTable->getCellFact(index.row(), index.column(), property, cellValue, factFound);

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (factFound) {
                // Pull the property value from the meta object system
                const QMetaObject* metaObject = fact.metaObject();
                int propertyIndex = metaObject->indexOfProperty(property.toAscii().constData());
                Q_ASSERT(propertyIndex);
                QMetaProperty metaProperty = metaObject->property(propertyIndex);
                retVariant = metaProperty.read(&fact);
                
                float floatValue = retVariant.toFloat();
                if (isnan(floatValue)) {
                    Q_ASSERT(role != Qt::EditRole);
                    retVariant = QVariant(QString("[unavailable] " + fact.units()));
                } else if (metaProperty.isUser() && role == Qt::DisplayRole) {
                    // Translate value and/or add units to textual representation
                    QString label;
                    QString valueStr;
                    bool found = fact.lookupValueLabel(floatValue, label);
                    if (found) {
                        valueStr = label;
                    } else {
                        valueStr = retVariant.toString();
                    }
                    retVariant = valueStr + " " + fact.units();
                }
                
            } else {
                // No fact in cell, just display text
                retVariant = cellValue;
            }
        } else if (role == Qt::BackgroundRole || role == Qt::ForegroundRole) {
            if (factFound) {
                // Determine if this is the USER property
                const QMetaObject* metaObject = fact.metaObject();
                int propertyIndex = metaObject->indexOfProperty(property.toAscii().constData());
                Q_ASSERT(propertyIndex);
                QMetaProperty metaProperty = metaObject->property(propertyIndex);

                if (metaProperty.isUser()) {
                    if (role == Qt::BackgroundRole) {
                        if (fact.isModified()) {
                            retVariant = QVariant(QBrush(QColor(255, 0, 0)));
                        } else {
                            if (fact.hasDefaultValue() && (fact.value() != fact.defaultValue())) {
                                retVariant = QVariant(QBrush(QColor(255, 255, 0)));
                            }
                        }
                    } else if (role == Qt::ForegroundRole) {
                        if (fact.isModified()) {
                            retVariant = QVariant(QBrush(QColor(255, 255, 255)));
                        } else {
                            if (fact.hasDefaultValue() && (fact.value() != fact.defaultValue())) {
                                retVariant = QVariant(QBrush(QColor(0, 0, 0)));
                            }
                        }
                    }
                }
            }
        } else if (role == Qt::ToolTipRole) {
            if (factFound) {
                retVariant = fact.description();
            }
        }
    } else if (role == Qt::FontRole) {
        //retVariant = QVariant(QFont("LucidaGrande", 8));
    }
    
    return retVariant;
}

QVariant FactTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return QVariant(_driverTable->getColumnLabel(section));
    }
    
    // return invalid variant for all other role values
    return QVariant();
}

bool FactTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    //qDebug() << "setData" << Qt::EditRole << role;

    if (role == Qt::EditRole) {
        QString property;
        QString cellValue;
        bool    factFound;
        
        Fact& fact = _driverTable->getCellFact(index.row(), index.column(), property, cellValue, factFound);

        Q_ASSERT(factFound);

        bool converted;
        float newValue = value.toFloat(&converted);
        if (converted && newValue != fact.value()) {
            fact.setValue(newValue);
            fact.setModified(true);
            
            // Send the new parameter out
            Q_ASSERT(_uas);
            _uas->getParamManager()->setParameter(1, fact.id(), fact.value());

            emit dataChanged(index, index);
        }
        
        return true;
    }
    
    return false;
}

Qt::ItemFlags FactTableModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    QString property;
    QString cellValue;
    bool    factFound;
    
    Fact& fact= _driverTable->getCellFact(index.row(), index.column(), property, cellValue, factFound);
    
    if (factFound) {
        // Determine if this is the USER property
        const QMetaObject* metaObject = fact.metaObject();
        int propertyIndex = metaObject->indexOfProperty(property.toAscii().constData());
        Q_ASSERT(propertyIndex);
        QMetaProperty metaProperty = metaObject->property(propertyIndex);

        if (metaProperty.isUser() && !fact.isReadOnly()) {
            //qDebug() << fact.id() << "editable";
            flags |= Qt::ItemIsEditable;
        }
    }

    return flags;
}

void FactTableModel::_factUpdated(int uasId, Fact::Provider_t provider, const QString& factId)
{
    Q_UNUSED(factId);
    Q_UNUSED(provider);
    
    int row, col;
    
    Q_ASSERT(_uas);
    if (_uas->getUASID() == uasId && _driverTable->findValueCell(factId, provider, row, col)) {
        emit dataChanged(index(row, col), index(row, col));
    }
}
