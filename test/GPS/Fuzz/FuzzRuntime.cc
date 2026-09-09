#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    static QCoreApplication application(*argc, *argv);
    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false\n*.info=false\n*.warning=false"));
    return 0;
}
