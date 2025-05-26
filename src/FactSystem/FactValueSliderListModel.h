/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

class Fact;

Q_DECLARE_LOGGING_CATEGORY(FactValueSliderListModelLog)

/// Provides a list model of values for incrementing/decrementing the value of a Fact
class FactValueSliderListModel : public QAbstractListModel
{
    Q_OBJECT
    /// The initial value of the Fact at the meta data specified decimal place precision
    Q_PROPERTY(double initialValueAtPrecision READ initialValueAtPrecision NOTIFY initialValueAtPrecisionChanged)

public:
    explicit FactValueSliderListModel(const Fact &fact, QObject *parent = nullptr);
    ~FactValueSliderListModel();

    double initialValueAtPrecision() const { return _initialValueAtPrecision; }

    Q_INVOKABLE int resetInitialValue(void);
    Q_INVOKABLE double valueAtModelIndex(int index) const;
    Q_INVOKABLE int valueIndexAtModelIndex(int index) const;

signals:
    void initialValueAtPrecisionChanged();

private:
    double _valueAtPrecision(double value) const;

    int	rowCount(const QModelIndex &parent = QModelIndex()) const final { Q_UNUSED(parent); return _cValues; }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;
    QHash<int, QByteArray> roleNames() const final;

    const Fact &_fact;
    int _cValues = 0;
    int _firstValueIndexInWindow = 0;
    int _initialValueIndex = 0;
    int _cPrevValues = 0;
    int _cNextValues = 0;
    int _windowSize = 0;
    double _initialValue = 0;
    double _initialValueAtPrecision = 0;
    double _increment = 0;

    static constexpr int _valueRole = Qt::UserRole;
    static constexpr int _valueIndexRole = Qt::UserRole + 1;
};
