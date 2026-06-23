#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

class TestFixturesSignalEmitter : public QObject
{
    Q_OBJECT

public:
    explicit TestFixturesSignalEmitter(QObject* parent = nullptr) : QObject(parent) {}

    void emitValueChanged(int value)
    {
        emit valueChanged(value);
    }

    void emitStateChanged(bool state)
    {
        emit stateChanged(state);
    }

    void emitErrorOccurred(const QString& error)
    {
        emit errorOccurred(error);
    }

signals:
    void valueChanged(int value);
    void stateChanged(bool state);
    void errorOccurred(const QString& error);
};
