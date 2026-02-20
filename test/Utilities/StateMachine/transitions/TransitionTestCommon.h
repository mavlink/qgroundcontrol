#pragma once

#include "UnitTest.h"

#include "MultiSignalSpy.h"
#include "QGCStateMachine.h"

#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

// Test helper for SignalDataTransition tests
class SignalEmitter : public QObject
{
    Q_OBJECT
public:
    void emitValueChanged(const QString& value) { emit valueChanged(value); }
    void emitBoolChanged(bool value) { emit boolChanged(value); }
    void emitIntChanged(int value) { emit intChanged(value); }
    void emitDoubleChanged(double value) { emit doubleChanged(value); }
    void emitTwoInts(int a, int b) { emit twoIntsChanged(a, b); }
signals:
    void valueChanged(const QString& value);
    void boolChanged(bool value);
    void intChanged(int value);
    void doubleChanged(double value);
    void twoIntsChanged(int a, int b);
};
