#include "m4lib.h"

#include "m4serial.h"
#include "TyphoonHM4Interface.h"

#if defined(__androidx86__)
static const char* kUartName        = "/dev/ttyMFD0";
#endif

M4Lib::M4Lib(QObject* parent)
    : QObject(parent)
{
    _commPort = new M4SerialComm(this);
}

//-----------------------------------------------------------------------------
M4Lib::~M4Lib()
{
    if(_commPort) {
        delete _commPort;
    }
}

void
M4Lib::init()
{
#if defined(__androidx86__)
    if(!_commPort || !_commPort->init(kUartName, 230400) || !_commPort->open()) {
        //-- TODO: If this ever happens, we need to do something about it
        qCWarning(YuneecLog) << "Could not start serial communication with M4";
    } else {
        connect(_commPort, &M4SerialComm::bytesReady, this, &M4Lib::_bytesReady);
    }
#endif
}

void
M4Lib::deinit()
{
    disconnect(_commPort, &M4SerialComm::bytesReady, this, &M4Lib::_bytesReady);
}

bool M4Lib::write(QByteArray data, bool debug)
{
    return _commPort->write(data, debug);
}

void M4Lib::tryRead()
{
    _commPort->tryRead();
}

void M4Lib::_bytesReady(QByteArray data)
{
    emit(bytesReady(data));
}
