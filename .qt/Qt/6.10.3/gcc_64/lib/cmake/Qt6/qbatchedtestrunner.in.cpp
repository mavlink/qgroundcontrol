// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QList>
#include <QString>
#include <QtTest/private/qtestcase_p.h>

int main(int argc, char **argv)
{
    if (argc == 1) {
        printf("%s\n", QTest::qGetTestCaseNames().join(
            QStringLiteral(" ")).toStdString().c_str());
        return 0;
    }

    const auto entryFunction = QTest::qGetTestCaseEntryFunction(QString::fromUtf8(argv[1]));
    return entryFunction ? entryFunction(argc - 1, argv + 1) : -1;
}
