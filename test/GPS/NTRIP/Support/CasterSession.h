#pragma once

#include <QtTest/QSignalSpy>

#include "ManualScheduler.h"
#include "MultiSignalSpy.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPHttpTransport.h"

namespace GPSTest {

/// A plain-TCP loopback caster and an NTRIPHttpTransport for it on a manual scheduler, recording every transport
/// signal from construction on.
class CasterSession
{
public:
    CasterSession()
        : transport(caster.connectionConfig(), {}, nullptr, &scheduler)
    {
        (void) spy.init(&transport);
    }

    CasterSession(const CasterSession&) = delete;
    CasterSession& operator=(const CasterSession&) = delete;

    /// Starts the transport and returns the caster side of its connection, or null when none arrives.
    [[nodiscard]] ScriptedNTRIPCaster::Connection* start()
    {
        if (!caster.isListening() || !spy.isValid()) {
            return nullptr;
        }
        transport.start();
        auto* connection = caster.waitForConnection();
        return connection && connection->peer ? connection : nullptr;
    }

    /// The record of the transport signal @a name.
    [[nodiscard]] const QSignalSpy& signal(const char* name) const { return *spy.spy(name); }

    ScriptedNTRIPCaster caster;
    ManualScheduler scheduler;
    NTRIPHttpTransport transport;
    MultiSignalSpy spy;
};

}  // namespace GPSTest
