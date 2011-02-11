#ifndef SLUGSMAVUNITTEST_H
#define SLUGSMAVUNITTEST_H

#include <QObject>
#include <QtCore/QString>
#include <QtTest/QtTest>
#include "UAS.h"
#include "MAVLinkProtocol.h"
#include "UASInterface.h"
#include "AutoTest.h"
#include "SlugsMAV.h"

class SlugsMavUnitTest : public QObject
{
    Q_OBJECT
public:
#define UASID 5
    MAVLinkProtocol* mav;
    SlugsMAV* slugsMav;
  SlugsMavUnitTest();
signals:

private slots:
  void initTestCase();
  void cleanupTestCase();
  void first_test();

};

DECLARE_TEST(SlugsMavUnitTest)

#endif // SLUGSMAVUNITTEST_H
