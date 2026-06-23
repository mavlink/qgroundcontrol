#include "FactValueSliderListModelTest.h"

#include <QtCore/QAbstractListModel>

#include "Fact.h"
#include "FactMetaData.h"
#include "FactValueSliderListModel.h"

static Fact *_createDoubleFact(double value, double min, double max, double increment, int decimals,
                               QObject *parent)
{
    Fact *fact = new Fact(0, "slider", FactMetaData::valueTypeDouble, parent);
    FactMetaData *meta = fact->metaData();
    meta->setRawMin(QVariant(min));
    meta->setRawMax(QVariant(max));
    meta->setDecimalPlaces(decimals);
    meta->setRawIncrement(increment);
    fact->setRawValue(QVariant(value));
    return fact;
}

// Access private rowCount/roleNames through the public QAbstractListModel interface
static int _rowCount(QAbstractListModel &model)
{
    return model.rowCount();
}

static QHash<int, QByteArray> _roleNames(QAbstractListModel &model)
{
    return model.roleNames();
}

void FactValueSliderListModelTest::_resetInitialValueBasic_test()
{
    Fact *fact = _createDoubleFact(50.0, 0.0, 100.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    const int initialIndex = model.resetInitialValue();

    // prevValues = min((50-0)/1, 100) = 50
    // nextValues = min((100-50)/1, 100) = 50
    // total = 50 + 1 + 50 = 101
    QCOMPARE(_rowCount(model), 101);
    QCOMPARE(initialIndex, 50);
}

void FactValueSliderListModelTest::_rowCountMatchesTotalValues_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();

    // prevValues = 5, nextValues = 5, total = 11
    QCOMPARE(_rowCount(model), 11);
}

void FactValueSliderListModelTest::_valueAtInitialIndex_test()
{
    Fact *fact = _createDoubleFact(50.0, 0.0, 100.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    const int initialIndex = model.resetInitialValue();
    const double valueAtInitial = model.valueAtModelIndex(initialIndex);

    QCOMPARE_FUZZY(valueAtInitial, 50.0, 0.01);
    QCOMPARE_FUZZY(model.initialValueAtPrecision(), 50.0, 0.01);
}

void FactValueSliderListModelTest::_valueAtModelIndexBoundaries_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();

    // First index (index 0) should be near min
    const double first = model.valueAtModelIndex(0);
    QCOMPARE_FUZZY(first, 0.0, 0.01);

    // Last index should be near max
    const double last = model.valueAtModelIndex(_rowCount(model) - 1);
    QCOMPARE_FUZZY(last, 10.0, 0.01);
}

void FactValueSliderListModelTest::_valueIndexAtModelIndex_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();

    QCOMPARE(model.valueIndexAtModelIndex(0), 0);
    QCOMPARE(model.valueIndexAtModelIndex(5), 5);
    QCOMPARE(model.valueIndexAtModelIndex(10), 10);
}

void FactValueSliderListModelTest::_roleNames_test()
{
    Fact *fact = _createDoubleFact(0.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    const QHash<int, QByteArray> roles = _roleNames(model);
    QVERIFY(roles.values().contains("value"));
    QVERIFY(roles.values().contains("valueIndex"));
}

void FactValueSliderListModelTest::_resetClearsOldRows_test()
{
    Fact *fact = _createDoubleFact(5.0, 0.0, 10.0, 1.0, 0, this);
    FactValueSliderListModel model(*fact);

    model.resetInitialValue();
    const int firstCount = _rowCount(model);
    QVERIFY(firstCount > 0);

    // Change value and reset — should get fresh row count
    fact->setRawValue(QVariant(2.0));
    model.resetInitialValue();

    // prevValues = 2, nextValues = 8, total = 11
    QCOMPARE(_rowCount(model), 11);
}

void FactValueSliderListModelTest::_integerIncrement_test()
{
    Fact *fact = _createDoubleFact(50.0, 0.0, 100.0, 5.0, 0, this);
    FactValueSliderListModel model(*fact);

    const int initialIndex = model.resetInitialValue();

    // prevValues = min((50-0)/5, 100) = 10
    // nextValues = min((100-50)/5, 100) = 10
    // total = 10 + 1 + 10 = 21
    QCOMPARE(_rowCount(model), 21);

    // Values should step by 5
    const double v0 = model.valueAtModelIndex(0);
    const double v1 = model.valueAtModelIndex(1);
    QCOMPARE_FUZZY(v1 - v0, 5.0, 0.01);

    QCOMPARE_FUZZY(model.valueAtModelIndex(initialIndex), 50.0, 0.01);
}

UT_REGISTER_TEST(FactValueSliderListModelTest, TestLabel::Unit)
