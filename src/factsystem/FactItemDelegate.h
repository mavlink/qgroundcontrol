#ifndef FACTITEMDELEGATE_H
#define FACTITEMDELEGATE_H

#include <QItemDelegate>
#include "Fact.h"
#include "FactHandler.h"
#include <QTableWidget>

class FactTableWidget;
class UASInterface;

class FactItemDelegate : public QItemDelegate {
    Q_OBJECT
    
public:
    FactItemDelegate(UASInterface* uas, FactTableWidget* driverTable, QObject* parent = NULL);
    
    // overrides from QItemDelegate
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    
private:
    typedef enum {
        None,
        Combo,
        CheckCombo,
        SpinInt,
        SpinFloat,
        SpinDouble
    } CustomEditor_t;
    
    CustomEditor_t _customEditor(Fact& fact) const;
    Fact& _getFactFromDriver(int row, int col) const;

    UASInterface*       _uas;
    FactTableWidget*    _driverTable;
};

#endif