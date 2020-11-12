/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FactValueSliderListModel.h"

#include <QDebug>
#include <QQmlEngine>
#include <QtMath>

#include <math.h>

const int FactValueSliderListModel::_valueRole =        Qt::UserRole;
const int FactValueSliderListModel::_valueIndexRole =   Qt::UserRole + 1;

FactValueSliderListModel::FactValueSliderListModel(Fact& fact, QObject* parent)
    : QAbstractListModel        (parent)
    , _fact                     (fact)
    , _cValues                  (0)
    , _firstValueIndexInWindow  (0)
    , _initialValueIndex        (0)
    , _cPrevValues              (0)
    , _cNextValues              (0)
    , _initialValue             (0)
    , _initialValueAtPrecision  (0)
    , _increment                (0)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

FactValueSliderListModel::~FactValueSliderListModel()
{
}

int FactValueSliderListModel::resetInitialValue(void)
{
    if (_cValues > 0) {
        // Remove any old rows
        beginRemoveRows(QModelIndex(), 0, _cValues - 1);
        _cValues = 0;
        endRemoveRows();
    }

    _initialValue = _fact.cookedValue().toDouble();
    _initialValueAtPrecision = _valueAtPrecision(_initialValue);
    if (qRound(_fact.rawIncrement()) == _fact.rawIncrement()) {
        _increment = qRound(_fact.cookedIncrement());
    } else {
        _increment = _fact.cookedIncrement();
    }
    _cPrevValues = qMin((_initialValue - _fact.cookedMin().toDouble())  / _increment, 100.0);
    _cNextValues = qMin((_fact.cookedMax().toDouble() - _initialValue)  / _increment, 100.0);
    _initialValueIndex = _cPrevValues;

    int totalValueCount = _cPrevValues + 1 + _cNextValues;
    beginInsertRows(QModelIndex(), 0, totalValueCount - 1);
    _cValues = totalValueCount;
    endInsertRows();

    emit initialValueAtPrecisionChanged();

    return _initialValueIndex;
}

int FactValueSliderListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    
    return _cValues;
}

QVariant FactValueSliderListModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role);

    if (!index.isValid()) {
        return QVariant();
    }
    
    int valueIndex = index.row();
    if (valueIndex >= _cValues) {
        return QVariant();
    }

    if (role == _valueRole) {
        double value;
        int cIncrementCount = valueIndex - _initialValueIndex;
        if (cIncrementCount == 0) {
            value = _initialValue;
        } else {
            value = initialValueAtPrecision() + (cIncrementCount * _increment);
        }
        return QVariant(_valueAtPrecision(value));
    } else if (role == _valueIndexRole) {
        return QVariant::fromValue(valueIndex);
    } else {
        return QVariant();
    }


}

QHash<int, QByteArray> FactValueSliderListModel::roleNames(void) const
{
    QHash<int, QByteArray> hash;

    hash[_valueRole] = "value";
    hash[_valueIndexRole] = "valueIndex";

    return hash;
}

double FactValueSliderListModel::valueAtModelIndex(int index)
{
    return data(createIndex(index, 0), _valueRole).toDouble();

}

int FactValueSliderListModel::valueIndexAtModelIndex(int index)
{
    return data(createIndex(index, 0), _valueIndexRole).toInt();
}

double FactValueSliderListModel::_valueAtPrecision(double value) const
{
    double precision = qPow(10, _fact.decimalPlaces());
    return qRound(value * precision) / precision;
}
