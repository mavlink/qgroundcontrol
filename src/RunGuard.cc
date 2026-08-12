#include "RunGuard.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/types.h>
#include <signal.h>
#endif

namespace
{

QString generateKeyHash( const QString& key, const QString& salt )
{
    QByteArray data;

    data.append( key.toUtf8() );
    data.append( salt.toUtf8() );
    data = QCryptographicHash::hash( data, QCryptographicHash::Sha1 ).toHex();

    return data;
}

bool isPidAlive( qint64 pid )
{
    if ( pid <= 0 ) return false;
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>( pid ) );
    if ( hProcess == NULL ) {
        return false;
    }
    DWORD exitCode = 0;
    bool alive = GetExitCodeProcess( hProcess, &exitCode ) && ( exitCode == STILL_ACTIVE );
    CloseHandle( hProcess );
    return alive;
#else
    return ( kill( static_cast<pid_t>( pid ), 0 ) == 0 );
#endif
}

}

RunGuard::RunGuard( const QString& key )
    : key( key )
    , memLockKey( generateKeyHash( key, "_memLockKey" ) )
    , sharedmemKey( generateKeyHash( key, "_sharedmemKey" ) )
    , sharedMem( sharedmemKey )
    , memLock( memLockKey, 1 )
{
    memLock.acquire();
    {
        QSharedMemory fix( sharedmemKey );    // Fix for *nix: http://habrahabr.ru/post/173281/
        fix.attach();
    }
    memLock.release();
}

RunGuard::~RunGuard()
{
    release();
}

bool RunGuard::isAnotherRunning()
{
    if ( sharedMem.isAttached() )
        return false;

    memLock.acquire();
    bool isRunning = sharedMem.attach();
    if ( isRunning ) {
        qint64 runningPid = 0;
        if ( sharedMem.lock() ) {
            memcpy( &runningPid, sharedMem.constData(), sizeof( qint64 ) );
            sharedMem.unlock();
        }
        if ( !isPidAlive( runningPid ) ) {
            isRunning = false;
        }
        sharedMem.detach();
    }
    memLock.release();

    return isRunning;
}

bool RunGuard::tryToRun()
{
    memLock.acquire();
    bool result = sharedMem.create( sizeof( qint64 ) );
    if ( !result ) {
        // Cleanup stale shared memory segment from previous crashed/killed process
        if ( sharedMem.attach() ) {
            qint64 runningPid = 0;
            if ( sharedMem.lock() ) {
                memcpy( &runningPid, sharedMem.constData(), sizeof( qint64 ) );
                sharedMem.unlock();
            }

            if ( !isPidAlive( runningPid ) ) {
                sharedMem.detach();
                result = sharedMem.create( sizeof( qint64 ) );
            } else {
                sharedMem.detach();
                memLock.release();
                return false;
            }
        }
    }

    if ( result ) {
        qint64 currentPid = QCoreApplication::applicationPid();
        if ( sharedMem.lock() ) {
            memcpy( sharedMem.data(), &currentPid, sizeof( qint64 ) );
            sharedMem.unlock();
        }
    }

    memLock.release();
    if ( !result )
    {
        release();
        return false;
    }

    return true;
}

void RunGuard::release()
{
    memLock.acquire();
    if ( sharedMem.isAttached() )
        sharedMem.detach();
    memLock.release();
}
