#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>

namespace QGCUnitTest {

int runTests(bool stress, const QStringList& unitTests);

}
