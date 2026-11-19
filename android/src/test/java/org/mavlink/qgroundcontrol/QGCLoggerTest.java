package org.mavlink.qgroundcontrol;

import static org.junit.Assert.assertEquals;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.pm.ApplicationInfo;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.junit.After;
import org.junit.Test;

/** Unit tests for what {@link QGCLogger} hands to QGC's own log. What goes to Android's log is not seen here. */
public class QGCLoggerTest {
    private static final String TAG = "Tag";

    /** A sink that keeps what it is handed, with the kind of message in front. */
    private static final class RecordingSink implements QGCLogger.Sink {
        final List<String> messages = new ArrayList<>();

        @Override
        public void debug(final String message) {
            messages.add("debug " + message);
        }

        @Override
        public void warning(final String message) {
            messages.add("warning " + message);
        }
    }

    /** A sink that fails as the activity's does where the native library is not loaded, as it is not here. */
    private static final class FailingSink implements QGCLogger.Sink {
        int calls;

        @Override
        public void debug(final String message) {
            calls++;
            throw new UnsatisfiedLinkError("qgcLogDebug");
        }

        @Override
        public void warning(final String message) {
            calls++;
            throw new UnsatisfiedLinkError("qgcLogWarning");
        }
    }

    @After
    public void tearDown() {
        QGCLogger.setSink(null);
        QGCLogger.initialize(contextOfABuild(false));
    }

    @Test
    public void i_goesToTheSinkAsADebugMessageWithItsTag() {
        final RecordingSink sink = new RecordingSink();
        QGCLogger.setSink(sink);

        QGCLogger.i(TAG, "Opened port 1");

        assertEquals(List.of("debug Tag: Opened port 1"), sink.messages);
    }

    @Test
    public void wAndE_goToTheSinkAsWarnings_withWhatTheirThrowableSays() {
        final RecordingSink sink = new RecordingSink();
        QGCLogger.setSink(sink);

        QGCLogger.w(TAG, "No permission");
        QGCLogger.w(TAG, "Could not set the timer", new IOException("device gone"));
        QGCLogger.e(TAG, "No USB connection");
        QGCLogger.e(TAG, "Error closing port 1", new IllegalStateException());

        assertEquals(List.of("warning Tag: No permission",
                             "warning Tag: Could not set the timer: device gone",
                             "warning Tag: No USB connection",
                             "warning Tag: Error closing port 1: IllegalStateException"),
                     sink.messages);
    }

    @Test
    public void d_staysOutOfTheSink_alsoInABuildThatLogsIt() {
        final RecordingSink sink = new RecordingSink();
        QGCLogger.setSink(sink);

        QGCLogger.d(TAG, "not logged at all");
        QGCLogger.initialize(contextOfABuild(true));
        QGCLogger.d(TAG, "logged to Android only");

        assertEquals(List.of(), sink.messages);
    }

    @Test
    public void withoutASink_nothingIsHandedOn() {
        final RecordingSink sink = new RecordingSink();
        QGCLogger.setSink(sink);
        QGCLogger.setSink(null);

        // As before the activity exists and once it is gone.
        QGCLogger.i(TAG, "Opened port 1");
        QGCLogger.e(TAG, "Error closing port 1", new IOException("device gone"));

        assertEquals(List.of(), sink.messages);
    }

    @Test
    public void sinkThatFails_doesNotFailTheCallerAndIsNotAskedAgain() {
        final FailingSink sink = new FailingSink();
        QGCLogger.setSink(sink);

        QGCLogger.i(TAG, "first");
        QGCLogger.w(TAG, "second");
        QGCLogger.e(TAG, "third", new IOException("device gone"));

        assertEquals(1, sink.calls);
    }

    /** A context of a build that is debuggable, in which {@code d()} logs, or of one that is not. */
    private static Context contextOfABuild(final boolean debuggable) {
        final ApplicationInfo info = new ApplicationInfo();
        info.flags = debuggable ? ApplicationInfo.FLAG_DEBUGGABLE : 0;
        return new ContextWrapper(null) {
            @Override
            public ApplicationInfo getApplicationInfo() {
                return info;
            }
        };
    }
}
