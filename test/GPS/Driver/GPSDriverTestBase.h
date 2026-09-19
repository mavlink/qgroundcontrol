#pragma once

#include "PortableTest.h"

#ifdef QGC_PORTABLE_TEST
#include <QtCore/QRegularExpression>

class GPSDriverTestBase : public QObject
{
    Q_OBJECT

protected:
    void expectLogMessage(const char* category, QtMsgType type, const QRegularExpression& expression)
    {
        _expected.append({QString::fromUtf8(category), type, expression, false});
    }

    void verifyExpectedLogMessage()
    {
        QVERIFY(!_expected.isEmpty());
        const auto expected = _expected.takeFirst();
        QVERIFY2(expected.received, qPrintable(expected.expression.pattern()));
    }

private slots:

    void init()
    {
        _active = this;
        _previous = qInstallMessageHandler(_message);
    }

    void cleanup()
    {
        qInstallMessageHandler(_previous);
        _active = nullptr;
        const bool verified = _expected.isEmpty();
        _expected.clear();
        QVERIFY2(verified, "Expected log messages were not verified");
    }

private:
    struct Expected
    {
        QString category;
        QtMsgType type;
        QRegularExpression expression;
        bool received;
    };

    static void _message(QtMsgType type, const QMessageLogContext& context, const QString& text)
    {
        for (auto& expected : _active->_expected) {
            if (!expected.received && type == expected.type &&
                expected.category == QString::fromUtf8(context.category) &&
                expected.expression.match(text).hasMatch()) {
                expected.received = true;
                return;
            }
        }
        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
            QTest::qFail(qPrintable(text), __FILE__, __LINE__);
        } else if (_active->_previous) {
            _active->_previous(type, context, text);
        }
    }

    inline static GPSDriverTestBase* _active = nullptr;
    QtMessageHandler _previous = nullptr;
    QList<Expected> _expected;
};
#else
using GPSDriverTestBase = UnitTest;
#endif
