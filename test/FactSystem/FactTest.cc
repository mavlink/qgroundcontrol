#include "FactTest.h"

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "FactMetaData.h"

void FactTest::_constructWithTypeAndName_test()
{
    Fact fact(0, "TestParam", FactMetaData::valueTypeInt32);

    QCOMPARE(fact.name(), QStringLiteral("TestParam"));
    QCOMPARE(fact.componentId(), 0);
    QCOMPARE(fact.type(), FactMetaData::valueTypeInt32);
}

void FactTest::_setRawValueInt_test()
{
    Fact fact(0, "IntParam", FactMetaData::valueTypeInt32);

    fact.setRawValue(QVariant(42));
    QCOMPARE(fact.rawValue().toInt(), 42);

    fact.setRawValue(QVariant(-10));
    QCOMPARE(fact.rawValue().toInt(), -10);
}

void FactTest::_setRawValueDouble_test()
{
    Fact fact(0, "DblParam", FactMetaData::valueTypeDouble);

    fact.setRawValue(QVariant(3.14));
    QCOMPARE(fact.rawValue().toDouble(), 3.14);
}

void FactTest::_setRawValueString_test()
{
    Fact fact(0, "StrParam", FactMetaData::valueTypeString);

    fact.setRawValue(QVariant("hello"));
    QCOMPARE(fact.rawValue().toString(), QStringLiteral("hello"));
}

void FactTest::_setCookedValueWithTranslator_test()
{
    Fact fact(0, "RadParam", FactMetaData::valueTypeDouble);

    auto *meta = fact.metaData();
    QVERIFY(meta);
    meta->setRawUnits("radians");

    // Cooked units should now be "deg"
    QCOMPARE(fact.cookedUnits(), QStringLiteral("deg"));

    // Setting cooked value of 180 degrees -> raw value should be PI
    fact.setCookedValue(QVariant(180.0));
    QCOMPARE_FUZZY(fact.rawValue().toDouble(), M_PI, 1e-5);
    QCOMPARE_FUZZY(fact.cookedValue().toDouble(), 180.0, 1e-5);
}

void FactTest::_validateValid_test()
{
    Fact fact(0, "IntParam", FactMetaData::valueTypeInt32);

    auto *meta = fact.metaData();
    QVERIFY(meta);
    meta->setRawMin(QVariant(0));
    meta->setRawMax(QVariant(100));

    const QString error = fact.validate("50", false);
    QVERIFY(error.isEmpty());
}

void FactTest::_validateInvalid_test()
{
    Fact fact(0, "IntParam", FactMetaData::valueTypeInt32);

    auto *meta = fact.metaData();
    QVERIFY(meta);
    meta->setRawMin(QVariant(0));
    meta->setRawMax(QVariant(100));

    const QString error = fact.validate("200", false);
    QVERIFY(!error.isEmpty());
}

void FactTest::_validateConvertOnly_test()
{
    Fact fact(0, "IntParam", FactMetaData::valueTypeInt32);

    auto *meta = fact.metaData();
    QVERIFY(meta);
    meta->setRawMin(QVariant(0));
    meta->setRawMax(QVariant(100));

    // Out of range but convertOnly=true should pass
    const QString error = fact.validate("200", true);
    QVERIFY(error.isEmpty());
}

void FactTest::_clampOutOfRange_test()
{
    Fact fact(0, "IntParam", FactMetaData::valueTypeInt32);

    auto *meta = fact.metaData();
    QVERIFY(meta);
    meta->setRawMin(QVariant(0));
    meta->setRawMax(QVariant(100));

    QVariant clamped = fact.clamp("200");
    QCOMPARE(clamped.toInt(), 100);

    clamped = fact.clamp("-10");
    QCOMPARE(clamped.toInt(), 0);

    clamped = fact.clamp("50");
    QCOMPARE(clamped.toInt(), 50);
}

void FactTest::_enumOperations_test()
{
    Fact fact(0, "EnumParam", FactMetaData::valueTypeInt32);

    const QStringList strings = {"Off", "Low", "Medium", "High"};
    const QVariantList values = {QVariant(0), QVariant(1), QVariant(2), QVariant(3)};
    fact.setEnumInfo(strings, values);

    QCOMPARE(fact.enumStrings().count(), 4);
    QCOMPARE(fact.enumValues().count(), 4);

    // Set by index
    fact.setEnumIndex(2);
    QCOMPARE(fact.rawValue().toInt(), 2);
    QCOMPARE(fact.enumIndex(), 2);
    QCOMPARE(fact.enumStringValue(), QStringLiteral("Medium"));

    // Set by string
    fact.setEnumStringValue("High");
    QCOMPARE(fact.rawValue().toInt(), 3);
    QCOMPARE(fact.enumIndex(), 3);
}

void FactTest::_valueChangedSignal_test()
{
    Fact fact(0, "SigParam", FactMetaData::valueTypeInt32);

    QSignalSpy spy(&fact, &Fact::valueChanged);
    QVERIFY(spy.isValid());

    fact.setRawValue(QVariant(42));
    QCOMPARE(spy.count(), 1);

    fact.setRawValue(QVariant(99));
    QCOMPARE(spy.count(), 2);
}

void FactTest::_rawValueChangedSignal_test()
{
    Fact fact(0, "SigParam", FactMetaData::valueTypeInt32);

    QSignalSpy spy(&fact, &Fact::rawValueChanged);
    QVERIFY(spy.isValid());

    fact.setRawValue(QVariant(42));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 42);
}

void FactTest::_noSignalOnSameValue_test()
{
    Fact fact(0, "SigParam", FactMetaData::valueTypeInt32);

    fact.setRawValue(QVariant(42));

    QSignalSpy spy(&fact, &Fact::valueChanged);
    QVERIFY(spy.isValid());

    // Setting same value should not emit
    fact.setRawValue(QVariant(42));
    QCOMPARE(spy.count(), 0);
}

void FactTest::_typeIsString_test()
{
    Fact stringFact(0, "StrParam", FactMetaData::valueTypeString);
    QVERIFY(stringFact.typeIsString());
    QVERIFY(!stringFact.typeIsBool());

    Fact intFact(0, "IntParam", FactMetaData::valueTypeInt32);
    QVERIFY(!intFact.typeIsString());
}

void FactTest::_typeIsBool_test()
{
    Fact boolFact(0, "BoolParam", FactMetaData::valueTypeBool);
    QVERIFY(boolFact.typeIsBool());
    QVERIFY(!boolFact.typeIsString());
}

void FactTest::_valueEqualsDefault_test()
{
    Fact fact(0, "DefParam", FactMetaData::valueTypeInt32);

    auto *meta = fact.metaData();
    QVERIFY(meta);
    meta->setRawDefaultValue(QVariant(42));

    // Initially raw value is 0, default is 42
    QVERIFY(!fact.valueEqualsDefault());

    fact.setRawValue(QVariant(42));
    QVERIFY(fact.valueEqualsDefault());

    fact.setRawValue(QVariant(99));
    QVERIFY(!fact.valueEqualsDefault());
}

UT_REGISTER_TEST(FactTest, TestLabel::Unit)
