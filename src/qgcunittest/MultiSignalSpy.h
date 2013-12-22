#ifndef MULTISIGNALSPY_H
#define MULTISIGNALSPY_H

#include <QObject>
#include <QSignalSpy>

/// @file
///     @brief This class allows you to keep track of signal counts on a set of signals associated with an object.
///     Mainly used for writing object unit tests.
///
///     @author Don Gagne <don@thegagnes.com>

class MultiSignalSpy : public QObject
{
    Q_OBJECT
    
public:
    MultiSignalSpy(QObject* parent = NULL);
    ~MultiSignalSpy();

    bool init(QObject* signalEmitter, const char** rgSignals, size_t cSignals);

    bool checkSignalByMask(quint16 mask);
    bool checkOnlySignalByMask(quint16 mask);
    bool checkNoSignalByMask(quint16 mask);
    bool checkNoSignals(void);

    void clearSignalByIndex(quint16 index);
    void clearSignalsByMask(quint16 mask);
    void clearAllSignals(void);

    bool waitForSignalByIndex(quint16 index, int msec);
    
    QSignalSpy* getSpyByIndex(quint16 index);
    
private:
    // QObject overrides
    void timerEvent(QTimerEvent * event);

    QObject*        _signalEmitter;
    const char**    _rgSignals;
    QSignalSpy**    _rgSpys;
    size_t          _cSignals;
    bool            _timeout;
};

#endif
