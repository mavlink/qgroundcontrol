#include "Viewer3DInstancingTest.h"
#include "Viewer3DInstancing.h"

#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>
#include <QtQuick3D/QQuick3DInstancing>
#include <QtTest/QSignalSpy>

#include <cmath>

static const int kEntrySize = static_cast<int>(sizeof(QQuick3DInstancing::InstanceTableEntry));

void Viewer3DInstancingTest::_testInitialState()
{
    Viewer3DInstancing inst;

    QCOMPARE(inst.count(), 0);

    int instanceCount = -1;
    QByteArray buf = inst.getInstanceBuffer(&instanceCount);
    QCOMPARE(instanceCount, 0);
    QVERIFY(buf.isEmpty());
}

void Viewer3DInstancingTest::_testAddEntry()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(1, 2, 3),
                  QVector3D(0.1f, 0.1f, 0.1f),
                  QQuaternion(),
                  QColor(Qt::red));

    QCOMPARE(inst.count(), 1);
    QCOMPARE(inst._entries.size(), 1);
    QCOMPARE(inst._entries[0].position, QVector3D(1, 2, 3));
    QCOMPARE(inst._entries[0].color, QColor(Qt::red));
}

void Viewer3DInstancingTest::_testAddMultipleEntries()
{
    Viewer3DInstancing inst;

    for (int i = 0; i < 5; ++i) {
        inst.addEntry(QVector3D(i, 0, 0),
                      QVector3D(1, 1, 1),
                      QQuaternion(),
                      QColor(Qt::blue));
    }

    QCOMPARE(inst.count(), 5);
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(inst._entries[i].position.x(), static_cast<float>(i));
    }
}

void Viewer3DInstancingTest::_testClear()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(1, 2, 3), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::red));
    inst.addEntry(QVector3D(4, 5, 6), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::green));
    QCOMPARE(inst.count(), 2);

    inst.clear();
    QCOMPARE(inst.count(), 0);
    QVERIFY(inst._dirty);
}

void Viewer3DInstancingTest::_testClearEmitsCountChanged()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(1, 2, 3), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::red));
    inst.addEntry(QVector3D(4, 5, 6), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::green));

    QSignalSpy spy(&inst, &Viewer3DInstancing::countChanged);
    QVERIFY(spy.isValid());

    inst.clear();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(inst.count(), 0);
}

void Viewer3DInstancingTest::_testGetInstanceBuffer()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(0, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::white));
    inst.addEntry(QVector3D(10, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::black));

    int instanceCount = -1;
    QByteArray buf = inst.getInstanceBuffer(&instanceCount);

    QCOMPARE(instanceCount, 2);
    QCOMPARE(buf.size(), 2 * kEntrySize);
}

void Viewer3DInstancingTest::_testGetInstanceBufferCaching()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(0, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::white));

    int count1 = -1;
    QByteArray buf1 = inst.getInstanceBuffer(&count1);
    QVERIFY(!inst._dirty);

    int count2 = -1;
    QByteArray buf2 = inst.getInstanceBuffer(&count2);

    QCOMPARE(buf1, buf2);
    QCOMPARE(count1, count2);
}

void Viewer3DInstancingTest::_testGetInstanceBufferNullCount()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(0, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::white));

    // Passing nullptr for instanceCount should not crash
    QByteArray buf = inst.getInstanceBuffer(nullptr);
    QCOMPARE(buf.size(), kEntrySize);
}

void Viewer3DInstancingTest::_testAddLineSegment()
{
    Viewer3DInstancing inst;

    // Horizontal segment along X: (0,0,0) -> (100,0,0)
    inst.addLineSegment(QVector3D(0, 0, 0), QVector3D(100, 0, 0), 10.0f, QColor(Qt::red));

    QCOMPARE(inst.count(), 1);

    const auto &entry = inst._entries[0];

    // Midpoint
    QCOMPARE_FUZZY(entry.position.x(), 50.0f, 0.01f);
    QCOMPARE_FUZZY(entry.position.y(), 0.0f, 0.01f);
    QCOMPARE_FUZZY(entry.position.z(), 0.0f, 0.01f);

    // Scale: radius = lineWidth * 0.1 = 1.0, distance = 100
    // X: 1.0/50 = 0.02, Y: 100/100 = 1.0, Z: 1.0/50 = 0.02
    QCOMPARE_FUZZY(entry.scale.x(), 0.02f, 0.001f);
    QCOMPARE_FUZZY(entry.scale.y(), 1.0f, 0.001f);
    QCOMPARE_FUZZY(entry.scale.z(), 0.02f, 0.001f);

    // Rotation should take Y-axis (0,1,0) to X-axis (1,0,0) => 90° around -Z
    QVector3D rotated = entry.rotation.rotatedVector(QVector3D(0, 1, 0));
    QCOMPARE_FUZZY(rotated.x(), 1.0f, 0.01f);
    QCOMPARE_FUZZY(rotated.y(), 0.0f, 0.01f);
    QCOMPARE_FUZZY(rotated.z(), 0.0f, 0.01f);
}

void Viewer3DInstancingTest::_testAddLineSegmentVertical()
{
    Viewer3DInstancing inst;

    // Vertical segment along Y: (0,0,0) -> (0,50,0)
    inst.addLineSegment(QVector3D(0, 0, 0), QVector3D(0, 50, 0), 5.0f, QColor(Qt::green));

    QCOMPARE(inst.count(), 1);

    const auto &entry = inst._entries[0];

    // Midpoint
    QCOMPARE_FUZZY(entry.position.y(), 25.0f, 0.01f);

    // Rotation: Y-axis to Y-axis => identity
    QVector3D rotated = entry.rotation.rotatedVector(QVector3D(0, 1, 0));
    QCOMPARE_FUZZY(rotated.x(), 0.0f, 0.01f);
    QCOMPARE_FUZZY(rotated.y(), 1.0f, 0.01f);
    QCOMPARE_FUZZY(rotated.z(), 0.0f, 0.01f);
}

void Viewer3DInstancingTest::_testAddLineSegmentDegenerate()
{
    Viewer3DInstancing inst;

    // Zero-length segment: no entry should be added
    inst.addLineSegment(QVector3D(5, 5, 5), QVector3D(5, 5, 5), 10.0f, QColor(Qt::blue));

    QCOMPARE(inst.count(), 0);
}

void Viewer3DInstancingTest::_testAddLineSegmentDiagonal()
{
    Viewer3DInstancing inst;

    // Diagonal segment: (0,0,0) -> (100,100,100)
    inst.addLineSegment(QVector3D(0, 0, 0), QVector3D(100, 100, 100), 5.0f, QColor(Qt::cyan));

    QCOMPARE(inst.count(), 1);

    const auto &entry = inst._entries[0];

    // Midpoint should be (50, 50, 50)
    QCOMPARE_FUZZY(entry.position.x(), 50.0f, 0.01f);
    QCOMPARE_FUZZY(entry.position.y(), 50.0f, 0.01f);
    QCOMPARE_FUZZY(entry.position.z(), 50.0f, 0.01f);

    // Distance = sqrt(3) * 100 ≈ 173.2
    float expectedDistance = std::sqrt(3.0f) * 100.0f;
    QCOMPARE_FUZZY(entry.scale.y(), expectedDistance / 100.0f, 0.01f);
}

void Viewer3DInstancingTest::_testSelectedIndexDefault()
{
    Viewer3DInstancing inst;

    QCOMPARE(inst.selectedIndex(), -1);
}

void Viewer3DInstancingTest::_testSelectedIndexHighlight()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(0, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::red));
    inst.addEntry(QVector3D(10, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::blue));
    inst.addEntry(QVector3D(20, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::green));

    inst.setSelectedIndex(1);
    QCOMPARE(inst.selectedIndex(), 1);

    int instanceCount = -1;
    QByteArray buf = inst.getInstanceBuffer(&instanceCount);
    QCOMPARE(instanceCount, 3);

    // Verify selected entry uses highlight color by rebuilding with no selection
    inst.setSelectedIndex(-1);
    QByteArray bufNoSelection = inst.getInstanceBuffer(&instanceCount);

    // Buffers must differ (entry 1 color changed)
    QVERIFY(buf != bufNoSelection);
}

void Viewer3DInstancingTest::_testSelectedIndexOutOfRange()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(0, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::red));
    inst.addEntry(QVector3D(10, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::blue));

    // Out of range index — should not crash
    inst.setSelectedIndex(999);
    QCOMPARE(inst.selectedIndex(), 999);

    int instanceCount = -1;
    QByteArray buf = inst.getInstanceBuffer(&instanceCount);
    QCOMPARE(instanceCount, 2);

    // No entry has index 999, so buffer should match no-selection output
    inst.setSelectedIndex(-1);
    QByteArray bufNoSelection = inst.getInstanceBuffer(&instanceCount);
    QCOMPARE(buf, bufNoSelection);
}

void Viewer3DInstancingTest::_testSelectedIndexSameValueNoop()
{
    Viewer3DInstancing inst;

    inst.addEntry(QVector3D(0, 0, 0), QVector3D(1, 1, 1), QQuaternion(), QColor(Qt::red));
    inst.setSelectedIndex(0);

    QSignalSpy spy(&inst, &Viewer3DInstancing::selectedIndexChanged);
    QVERIFY(spy.isValid());

    // Setting the same index should not emit
    inst.setSelectedIndex(0);
    QCOMPARE(spy.count(), 0);
}

UT_REGISTER_TEST(Viewer3DInstancingTest, TestLabel::Unit)
