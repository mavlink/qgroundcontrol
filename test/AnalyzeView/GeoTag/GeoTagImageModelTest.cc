#include "GeoTagImageModelTest.h"
#include "GeoTagImageModel.h"

#include <QtTest/QSignalSpy>
#include <QtPositioning/QGeoCoordinate>

void GeoTagImageModelTest::_addImageTest()
{
    GeoTagImageModel model;

    QSignalSpy countSpy(&model, &GeoTagImageModel::countChanged);

    QCOMPARE(model.count(), 0);
    QCOMPARE(model.rowCount(), 0);

    model.addImage("/path/to/image1.jpg");
    QCOMPARE(model.count(), 1);
    QCOMPARE(countSpy.count(), 1);

    model.addImage("/path/to/image2.jpg");
    QCOMPARE(model.count(), 2);
    QCOMPARE(countSpy.count(), 2);

    // Verify data
    QModelIndex idx0 = model.index(0);
    QCOMPARE(model.data(idx0, GeoTagImageModel::FilePathRole).toString(), QString("/path/to/image1.jpg"));
    QCOMPARE(model.data(idx0, GeoTagImageModel::FileNameRole).toString(), QString("image1.jpg"));
    QCOMPARE(model.data(idx0, GeoTagImageModel::StatusRole).toInt(), static_cast<int>(GeoTagImageModel::Pending));

    QModelIndex idx1 = model.index(1);
    QCOMPARE(model.data(idx1, GeoTagImageModel::FilePathRole).toString(), QString("/path/to/image2.jpg"));
    QCOMPARE(model.data(idx1, GeoTagImageModel::FileNameRole).toString(), QString("image2.jpg"));
}

void GeoTagImageModelTest::_clearTest()
{
    GeoTagImageModel model;

    model.addImage("/path/to/image1.jpg");
    model.addImage("/path/to/image2.jpg");
    QCOMPARE(model.count(), 2);

    QSignalSpy countSpy(&model, &GeoTagImageModel::countChanged);

    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(countSpy.count(), 1);

    // Clear on empty model should not emit
    model.clear();
    QCOMPARE(countSpy.count(), 1);
}

void GeoTagImageModelTest::_setStatusTest()
{
    GeoTagImageModel model;

    model.addImage("/path/to/image.jpg");

    QSignalSpy dataChangedSpy(&model, &GeoTagImageModel::dataChanged);

    model.setStatus(0, GeoTagImageModel::Processing);
    QCOMPARE(dataChangedSpy.count(), 1);

    QModelIndex idx = model.index(0);
    QCOMPARE(model.data(idx, GeoTagImageModel::StatusRole).toInt(), static_cast<int>(GeoTagImageModel::Processing));
    QCOMPARE(model.data(idx, GeoTagImageModel::StatusStringRole).toString(), QString("Processing"));

    // Set with error message
    model.setStatus(0, GeoTagImageModel::Failed, "Test error");
    QCOMPARE(dataChangedSpy.count(), 2);
    QCOMPARE(model.data(idx, GeoTagImageModel::StatusRole).toInt(), static_cast<int>(GeoTagImageModel::Failed));
    QCOMPARE(model.data(idx, GeoTagImageModel::ErrorMessageRole).toString(), QString("Test error"));

    // Setting same status should not emit
    model.setStatus(0, GeoTagImageModel::Failed, "Test error");
    QCOMPARE(dataChangedSpy.count(), 2);
}

void GeoTagImageModelTest::_setStatusInvalidIndexTest()
{
    GeoTagImageModel model;

    model.addImage("/path/to/image.jpg");

    QSignalSpy dataChangedSpy(&model, &GeoTagImageModel::dataChanged);

    // Invalid indices should be ignored
    model.setStatus(-1, GeoTagImageModel::Tagged);
    model.setStatus(1, GeoTagImageModel::Tagged);
    model.setStatus(100, GeoTagImageModel::Tagged);

    QCOMPARE(dataChangedSpy.count(), 0);
}

void GeoTagImageModelTest::_setCoordinateTest()
{
    GeoTagImageModel model;

    model.addImage("/path/to/image.jpg");

    QSignalSpy dataChangedSpy(&model, &GeoTagImageModel::dataChanged);

    QGeoCoordinate coord(47.123, -122.456, 100.0);
    model.setCoordinate(0, coord);

    QCOMPARE(dataChangedSpy.count(), 1);

    QModelIndex idx = model.index(0);
    QGeoCoordinate retrieved = model.data(idx, GeoTagImageModel::CoordinateRole).value<QGeoCoordinate>();
    QCOMPARE(retrieved.latitude(), coord.latitude());
    QCOMPARE(retrieved.longitude(), coord.longitude());
    QCOMPARE(retrieved.altitude(), coord.altitude());

    // Setting same coordinate should not emit
    model.setCoordinate(0, coord);
    QCOMPARE(dataChangedSpy.count(), 1);

    // Invalid index should be ignored
    model.setCoordinate(-1, coord);
    model.setCoordinate(1, coord);
    QCOMPARE(dataChangedSpy.count(), 1);
}

void GeoTagImageModelTest::_setStatusByPathTest()
{
    GeoTagImageModel model;

    model.addImage("/path/to/image1.jpg");
    model.addImage("/path/to/image2.jpg");
    model.addImage("/path/to/image3.jpg");

    QSignalSpy dataChangedSpy(&model, &GeoTagImageModel::dataChanged);

    model.setStatusByPath("/path/to/image2.jpg", GeoTagImageModel::Tagged);
    QCOMPARE(dataChangedSpy.count(), 1);

    QModelIndex idx1 = model.index(1);
    QCOMPARE(model.data(idx1, GeoTagImageModel::StatusRole).toInt(), static_cast<int>(GeoTagImageModel::Tagged));

    // Other images should be unchanged
    QModelIndex idx0 = model.index(0);
    QModelIndex idx2 = model.index(2);
    QCOMPARE(model.data(idx0, GeoTagImageModel::StatusRole).toInt(), static_cast<int>(GeoTagImageModel::Pending));
    QCOMPARE(model.data(idx2, GeoTagImageModel::StatusRole).toInt(), static_cast<int>(GeoTagImageModel::Pending));

    // Non-existent path should be ignored
    model.setStatusByPath("/nonexistent/path.jpg", GeoTagImageModel::Failed);
    QCOMPARE(dataChangedSpy.count(), 1);
}

void GeoTagImageModelTest::_setAllStatusTest()
{
    GeoTagImageModel model;

    model.addImage("/path/to/image1.jpg");
    model.addImage("/path/to/image2.jpg");
    model.addImage("/path/to/image3.jpg");

    // Set individual statuses first
    model.setStatus(0, GeoTagImageModel::Tagged);
    model.setStatus(1, GeoTagImageModel::Failed, "Error");

    QSignalSpy dataChangedSpy(&model, &GeoTagImageModel::dataChanged);

    model.setAllStatus(GeoTagImageModel::Pending);
    QCOMPARE(dataChangedSpy.count(), 1);

    // All should be Pending with cleared error messages
    for (int i = 0; i < 3; ++i) {
        QModelIndex idx = model.index(i);
        QCOMPARE(model.data(idx, GeoTagImageModel::StatusRole).toInt(), static_cast<int>(GeoTagImageModel::Pending));
        QVERIFY(model.data(idx, GeoTagImageModel::ErrorMessageRole).toString().isEmpty());
    }

    // Empty model should not emit
    GeoTagImageModel emptyModel;
    QSignalSpy emptySpy(&emptyModel, &GeoTagImageModel::dataChanged);
    emptyModel.setAllStatus(GeoTagImageModel::Pending);
    QCOMPARE(emptySpy.count(), 0);
}

void GeoTagImageModelTest::_roleNamesTest()
{
    GeoTagImageModel model;

    QHash<int, QByteArray> roles = model.roleNames();

    QCOMPARE(roles.value(GeoTagImageModel::FileNameRole), QByteArray("fileName"));
    QCOMPARE(roles.value(GeoTagImageModel::FilePathRole), QByteArray("filePath"));
    QCOMPARE(roles.value(GeoTagImageModel::StatusRole), QByteArray("status"));
    QCOMPARE(roles.value(GeoTagImageModel::StatusStringRole), QByteArray("statusString"));
    QCOMPARE(roles.value(GeoTagImageModel::ErrorMessageRole), QByteArray("errorMessage"));
    QCOMPARE(roles.value(GeoTagImageModel::CoordinateRole), QByteArray("coordinate"));
}

void GeoTagImageModelTest::_dataInvalidIndexTest()
{
    GeoTagImageModel model;

    model.addImage("/path/to/image.jpg");

    // Invalid indices should return empty QVariant
    QVERIFY(!model.data(model.index(-1), GeoTagImageModel::FileNameRole).isValid());
    QVERIFY(!model.data(model.index(1), GeoTagImageModel::FileNameRole).isValid());
    QVERIFY(!model.data(model.index(100), GeoTagImageModel::FileNameRole).isValid());

    // Invalid role should return empty QVariant
    QVERIFY(!model.data(model.index(0), Qt::UserRole + 100).isValid());
}

UT_REGISTER_TEST(GeoTagImageModelTest, TestLabel::Unit, TestLabel::AnalyzeView)
