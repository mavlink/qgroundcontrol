package org.mavlink.qgroundcontrol;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public class QGCMediaStoreTest {
    @Test
    public void videoMimeType_matchesSupportedRecordingFormats() {
        final String[][] cases = {
            {"2026-09-10_12.00.00.123.mp4", "video/mp4"},
            {"2026-09-10_12.00.00.123.thermalVideo.mov", "video/quicktime"},
            {"video.MKV", "video/x-matroska"}
        };
        for (String[] entry : cases) {
            assertEquals(entry[0], entry[1], QGCMediaStore.videoMimeType(entry[0]));
        }
    }

    @Test
    public void videoMimeType_rejectsUnknownFormats() {
        for (String name : new String[]{null, "", "video", "video.mp4.tmp", "photo.jpg"}) {
            assertNull(QGCMediaStore.videoMimeType(name));
        }
    }

    @Test
    public void imageMimeType_matchesJpegSnapshots() {
        for (String name : new String[]{"2026-09-10_15.07.20.921.jpg", "photo.jpeg", "photo.JPG"}) {
            assertEquals(name, "image/jpeg", QGCMediaStore.imageMimeType(name));
        }
    }

    @Test
    public void imageMimeType_rejectsNonJpegFormats() {
        for (String name : new String[]{null, "", "photo", "photo.jpg.tmp", "photo.png", "video.mp4"}) {
            assertNull(QGCMediaStore.imageMimeType(name));
        }
    }
}
