/**
* @author Rob Caldecott
* @note This was obtained from http://qtcreator.blogspot.com/2010/04/sample-multiple-unit-test-project.html
*
*/

#include "AutoTest.h"
#include <QDebug>

#if 1
// This is all you need to run all the tests
TEST_MAIN
#else
// Or supply your own main function
int main(int argc, char *argv[])
{
    int failures = AutoTest::run(argc, argv);
    if (failures == 0)
    {
	qDebug() << "ALL TESTS PASSED";
    }
    else
    {
	qDebug() << failures << " TESTS FAILED!";
    }
    return failures;
}
#endif
