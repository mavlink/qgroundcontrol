#include "FactItemDelegate.h"
#include "FactTableWidget.h"
#include "UASInterface.h"
#include <QDebug>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>


FactItemDelegate::FactItemDelegate(UASInterface* uas, FactTableWidget* driverTable, QObject* parent) :
    QItemDelegate(parent),
    _uas(uas),
    _driverTable(driverTable)
{
    Q_ASSERT(driverTable);
}

FactItemDelegate::CustomEditor_t FactItemDelegate::_customEditor(Fact& fact) const
{
    if (fact.valueLabelsCount()) {
        return Combo;
    } else {
        switch (fact.type()) {

            case Fact::valueTypeUint8:
            case Fact::valueTypeInt8:
            case Fact::valueTypeUint16:
            case Fact::valueTypeInt16:
            case Fact::valueTypeUint32:
            case Fact::valueTypeInt32:
                return SpinInt;

            case Fact::valueTypeFloat:
                return SpinFloat;
                
            case Fact::valueTypeDouble:
                return SpinDouble;
                
            case Fact::valueTypeChar:
            case Fact::valueTypeUint64:
            case Fact::valueTypeInt64:
                return None;
        }
    }
    
    return None;
}

Fact& FactItemDelegate::_getFactFromDriver(int row, int col) const
{
    QString property;
    QString cellValue;
    bool    factFound;
    
    Fact& fact = _driverTable->getCellFact(row, col, property, cellValue, factFound);
    Q_ASSERT(factFound);
    
    return fact;
}

QWidget* FactItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Fact& fact = _getFactFromDriver(index.row(), index.column());
    
    switch (_customEditor(fact)) {
        case None:
            break;
        case Combo:
        {
            Q_ASSERT(fact.valueLabelsCount() > 0);
            QComboBox* comboBox = new QComboBox(parent);
            for (int i=0; i<fact.valueLabelsCount(); i++) {
                const Fact::ValueLabel_t& valueLabel = fact.valueLabelAt(i);

                comboBox->addItem(QString("%1 (%2)").arg(valueLabel.label).arg(valueLabel.value), QVariant(valueLabel.value));
            }
            return comboBox;
        }
        case SpinInt:
        {
            QSpinBox* spinBox = new QSpinBox(parent);
            spinBox->setAutoFillBackground(true);
            if (fact.hasRange()) {
                spinBox->setRange(fact.rangeMin(), fact.rangeMax());
            }
            spinBox->setSingleStep(fact.hasIncrement() ? fact.increment() : 1.0);
            spinBox->setSuffix(" " + fact.units());
            return spinBox;
        }
        case SpinFloat:
        case SpinDouble:
        {
            QDoubleSpinBox* doubleSpinBox = new QDoubleSpinBox(parent);
            doubleSpinBox->setAutoFillBackground(true);
            if (fact.hasRange()) {
                doubleSpinBox->setRange(fact.rangeMin(), fact.rangeMax());
            }
            doubleSpinBox->setSingleStep(fact.hasIncrement() ? fact.increment() : 1.0);
            doubleSpinBox->setSuffix(" " + fact.units());
            return doubleSpinBox;
        }
    }
        
    return QItemDelegate::createEditor(parent, option, index);
}

void FactItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    Fact& fact = _getFactFromDriver(index.row(), index.column());

    switch (_customEditor(fact)) {
        case None:
            break;
        case Combo:
        {
            int intValue = index.model()->data(index, Qt::EditRole).toInt();
            QComboBox *comboBox = dynamic_cast<QComboBox*>(editor);
            int index = comboBox->findData(QVariant(intValue));
            Q_ASSERT_X(index != -1, "setEditorData", "value not found");
            comboBox->setCurrentIndex(index);
            return;
        }
        case SpinInt:
        {
            int intValue = index.model()->data(index, Qt::EditRole).toInt();
            QSpinBox *spinBox = dynamic_cast<QSpinBox*>(editor);
            spinBox->setValue(intValue);
            return;
        }
        case SpinFloat:
        case SpinDouble:
        {
            double doubleValue = index.model()->data(index, Qt::EditRole).toDouble();
            QDoubleSpinBox *doubleSpinBox = dynamic_cast<QDoubleSpinBox*>(editor);
            doubleSpinBox->setValue(doubleValue);
        }
    }
    
    return QItemDelegate::setEditorData(editor, index);

}

void FactItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    Fact& fact = _getFactFromDriver(index.row(), index.column());
    
    switch (_customEditor(fact)) {
        case None:
            break;
        case Combo:
        {
            QComboBox *comboBox = dynamic_cast<QComboBox*>(editor);
            QVariant var = comboBox->itemData(comboBox->currentIndex());
            model->setData(index, var.toInt(), Qt::EditRole);
            return;
        }
        case SpinInt:
        {
            QSpinBox *spinBox = dynamic_cast<QSpinBox*>(editor);
            spinBox->interpretText();
            model->setData(index, spinBox->value(), Qt::EditRole);
            return;
        }
        case SpinFloat:
        case SpinDouble:
        {
            QDoubleSpinBox *doubleSpinBox = dynamic_cast<QDoubleSpinBox*>(editor);
            doubleSpinBox->interpretText();
            model->setData(index, doubleSpinBox->value(), Qt::EditRole);
            return;
        }
    }
    
    return QItemDelegate::setModelData(editor, model, index);
}

void FactItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    
    QRect rect = option.rect;
    
    QSize size = editor->sizeHint();
    rect.setWidth(qMax(size.width(), rect.width()));
    rect.setHeight(qMax(size.height(), rect.height()));

    editor->setGeometry(rect);
}