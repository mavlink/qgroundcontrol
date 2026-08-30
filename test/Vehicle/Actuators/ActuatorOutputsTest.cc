#include "ActuatorOutputsTest.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "ActuatorOutputs.h"

using namespace ActuatorOutputs;

namespace {

QJsonObject auxOutputFromFixture()
{
    QFile file(QStringLiteral(":/unittest/actuators.example.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonArray outputs = doc.object().value(QStringLiteral("outputs_v1")).toArray();
    for (const QJsonValue& outputVal : outputs) {
        const QJsonObject output = outputVal.toObject();
        if (output.value(QStringLiteral("label")).toString() == QLatin1String("AUX")) {
            return output;
        }
    }
    return {};
}

void addSubgroupFromJson(ActuatorOutput* output, const QJsonObject& subgroupJson)
{
    auto* subgroup = new ActuatorOutputSubgroup(output, subgroupJson.value(QStringLiteral("label")).toString());
    const QJsonArray channelParameters = subgroupJson.value(QStringLiteral("per-channel-parameters")).toArray();
    for (const QJsonValue& paramVal : channelParameters) {
        Parameter param;
        param.parse(paramVal);
        subgroup->addChannelConfig(
            new ChannelConfig(subgroup, param, ChannelConfig::Function::Unspecified, Condition()));
    }
    const QJsonArray channels = subgroupJson.value(QStringLiteral("channels")).toArray();
    for (const QJsonValue& channelVal : channels) {
        const QJsonObject channel = channelVal.toObject();
        subgroup->addChannel(new ActuatorOutputChannel(subgroup, channel.value(QStringLiteral("label")).toString(),
                                                       channel.value(QStringLiteral("param-index")).toInt(),
                                                       *subgroup->channelConfigs(), nullptr, [](Fact*) {}));
    }
    output->addSubgroup(subgroup);
}

ActuatorChannelRow* rowByLabel(ActuatorOutput* output, const QString& label)
{
    for (int i = 0; i < output->channelRows()->count(); i++) {
        auto* row = qobject_cast<ActuatorChannelRow*>(output->channelRows()->get(i));
        if (row && row->label() == label) {
            return row;
        }
    }
    return nullptr;
}

}  // namespace

void ActuatorOutputsTest::_auxTableUnionsDshotMin()
{
    const QJsonObject auxJson = auxOutputFromFixture();
    QVERIFY(!auxJson.isEmpty());

    ActuatorOutput output(this, QStringLiteral("AUX"), Condition());
    const QJsonArray subgroups = auxJson.value(QStringLiteral("subgroups")).toArray();
    for (const QJsonValue& subgroupVal : subgroups) {
        addSubgroupFromJson(&output, subgroupVal.toObject());
    }
    output.rebuildChannelRows();

    QCOMPARE(output.tableChannelConfigs()->count(), 6);
    auto* dshotMin = qobject_cast<ChannelConfig*>(output.tableChannelConfigs()->get(5));
    QVERIFY(dshotMin);
    QCOMPARE(dshotMin->parameter(), QStringLiteral("DSHOT_FMU_MIN${i}"));
    QCOMPARE(dshotMin->label(), QStringLiteral("Min"));

    const QStringList pwmOnly = {QStringLiteral("AUX 1"), QStringLiteral("AUX 4"), QStringLiteral("CAP 1")};
    const QStringList withDshot = {QStringLiteral("AUX 5"), QStringLiteral("AUX 6"), QStringLiteral("AUX 7"),
                                   QStringLiteral("AUX 8")};

    for (int i = 0; i < output.channelRows()->count(); i++) {
        auto* row = qobject_cast<ActuatorChannelRow*>(output.channelRows()->get(i));
        QVERIFY(row);
        QCOMPARE(row->configInstances()->count(), output.tableChannelConfigs()->count());
    }

    for (const QString& label : pwmOnly) {
        ActuatorChannelRow* row = rowByLabel(&output, label);
        QVERIFY2(row, qPrintable(label));
        auto* cell = qobject_cast<ActuatorChannelCell*>(row->configInstances()->get(5));
        QVERIFY(cell);
        QVERIFY(!cell->hasConfig());
    }
    for (const QString& label : withDshot) {
        ActuatorChannelRow* row = rowByLabel(&output, label);
        QVERIFY2(row, qPrintable(label));
        auto* cell = qobject_cast<ActuatorChannelCell*>(row->configInstances()->get(5));
        QVERIFY(cell);
        QVERIFY(cell->hasConfig());
        QCOMPARE(row->channel()->configInstances()->count(), 6);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(ActuatorOutputsTest, TestLabel::Unit, TestLabel::Vehicle)
