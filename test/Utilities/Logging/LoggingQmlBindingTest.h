#pragma once

#include <QtQml/QQmlEngine>

#include "UnitTest.h"

class LoggingQmlBindingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;

    void _singletonAccessible();
    void _modelRolesExposed();
    void _modelReceivesEntries();
    void _filterBindingsWork();
    void _settingsDialogLoads();

private:
    QQmlEngine* _engine = nullptr;
};
