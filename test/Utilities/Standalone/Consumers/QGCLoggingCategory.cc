#include "QGCLoggingCategory.h"

#include <QtCore/QCoreApplication>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QGCLoggingCategory category(QStringLiteral("Test.Consumer.Logging"));
    const auto snapshot = qgcObserveLoggingCategories(&app, [](const QString&) {});
    return snapshot.contains(QStringLiteral("Test.Consumer.Logging")) ? 0 : 1;
}
