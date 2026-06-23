package org.mavlink.qgroundcontrol;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.file.Files;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

/**
 * Unit tests for {@link QGCActivity} helpers that can be exercised without a
 * running Android runtime.
 */
public class QGCActivityTest {

    // -----------------------------------------------------------------------
    // isValidImportFileName  (covers the copyFileToDestination file-name gate)
    // -----------------------------------------------------------------------

    @Test
    public void isValidImportFileName_returnsTrueForLowerCasePlanExtension() {
        assertTrue(QGCActivity.isValidImportFileName("mission.plan"));
    }

    @Test
    public void isValidImportFileName_isCaseInsensitive() {
        assertTrue(QGCActivity.isValidImportFileName("MISSION.PLAN"));
        assertTrue(QGCActivity.isValidImportFileName("Mission.Plan"));
        assertTrue(QGCActivity.isValidImportFileName("test.PLAN"));
    }

    @Test
    public void isValidImportFileName_acceptsNamesWithSpacesAndSpecialChars() {
        assertTrue(QGCActivity.isValidImportFileName("my mission (v2).plan"));
        assertTrue(QGCActivity.isValidImportFileName("survey_grid.plan"));
    }

    @Test
    public void isValidImportFileName_returnsFalseForWrongExtension() {
        assertFalse(QGCActivity.isValidImportFileName("mission.kml"));
        assertFalse(QGCActivity.isValidImportFileName("mission.json"));
        assertFalse(QGCActivity.isValidImportFileName("mission.waypoints"));
        assertFalse(QGCActivity.isValidImportFileName("mission"));
    }

    @Test
    public void isValidImportFileName_returnsFalseForEmptyName() {
        assertFalse(QGCActivity.isValidImportFileName(""));
    }

    @Test
    public void isValidImportFileName_returnsFalseForNullName() {
        assertFalse(QGCActivity.isValidImportFileName(null));
    }

    @Test
    public void isValidImportFileName_returnsFalseForPlanAsPrefix() {
        // ".plan" must be the suffix, not just appear somewhere in the name.
        assertFalse(QGCActivity.isValidImportFileName("plan.kml"));
        assertFalse(QGCActivity.isValidImportFileName("mission.plan.bak"));
    }

    // -----------------------------------------------------------------------
    // resolveDestFile — unique-name resolution with _1, _2, … suffixes
    // -----------------------------------------------------------------------

    @Test
    public void resolveDestFile_returnsSameNameWhenFileDoesNotExist() throws IOException {
        final File tmpDir = Files.createTempDirectory("qgc_test").toFile();
        try {
            final File result = QGCActivity.resolveDestFile(tmpDir, "mission.plan");
            assertNotNull(result);
            assertEquals("mission.plan", result.getName());
        } finally {
            tmpDir.delete();
        }
    }

    @Test
    public void resolveDestFile_appendsSuffix1WhenOriginalExists() throws IOException {
        final File tmpDir = Files.createTempDirectory("qgc_test").toFile();
        try {
            assertTrue(new File(tmpDir, "mission.plan").createNewFile());
            final File result = QGCActivity.resolveDestFile(tmpDir, "mission.plan");
            assertNotNull(result);
            assertEquals("mission_1.plan", result.getName());
        } finally {
            final File[] files = tmpDir.listFiles();
            if (files != null) { for (File f : files) f.delete(); }
            tmpDir.delete();
        }
    }

    @Test
    public void resolveDestFile_appendsSuffix2WhenSuffix1AlsoExists() throws IOException {
        final File tmpDir = Files.createTempDirectory("qgc_test").toFile();
        try {
            assertTrue(new File(tmpDir, "mission.plan").createNewFile());
            assertTrue(new File(tmpDir, "mission_1.plan").createNewFile());
            final File result = QGCActivity.resolveDestFile(tmpDir, "mission.plan");
            assertNotNull(result);
            assertEquals("mission_2.plan", result.getName());
        } finally {
            final File[] files = tmpDir.listFiles();
            if (files != null) { for (File f : files) f.delete(); }
            tmpDir.delete();
        }
    }

    @Test
    public void resolveDestFile_handlesNameWithoutExtension() throws IOException {
        final File tmpDir = Files.createTempDirectory("qgc_test").toFile();
        try {
            assertTrue(new File(tmpDir, "noext").createNewFile());
            final File result = QGCActivity.resolveDestFile(tmpDir, "noext");
            assertNotNull(result);
            assertEquals("noext_1", result.getName());
        } finally {
            final File[] files = tmpDir.listFiles();
            if (files != null) { for (File f : files) f.delete(); }
            tmpDir.delete();
        }
    }

    @Test
    public void resolveDestFile_skipsMultipleExistingVariants() throws IOException {
        final File tmpDir = Files.createTempDirectory("qgc_test").toFile();
        try {
            assertTrue(new File(tmpDir, "survey.plan").createNewFile());
            assertTrue(new File(tmpDir, "survey_1.plan").createNewFile());
            assertTrue(new File(tmpDir, "survey_2.plan").createNewFile());
            assertTrue(new File(tmpDir, "survey_3.plan").createNewFile());
            final File result = QGCActivity.resolveDestFile(tmpDir, "survey.plan");
            assertNotNull(result);
            assertEquals("survey_4.plan", result.getName());
        } finally {
            final File[] files = tmpDir.listFiles();
            if (files != null) { for (File f : files) f.delete(); }
            tmpDir.delete();
        }
    }

    // -----------------------------------------------------------------------
    // jniOnImportResult — verifies the onImportResult JNI bridge declaration
    // -----------------------------------------------------------------------

    @Test
    public void onImportResult_isPublicNativeAndTakesStringReturnsVoid() throws NoSuchMethodException {
        final Method method = QGCActivity.class.getDeclaredMethod("onImportResult", String.class);

        assertTrue("onImportResult must be public",
                Modifier.isPublic(method.getModifiers()));
        assertTrue("onImportResult must be native (jniOnImportResult JNI bridge)",
                Modifier.isNative(method.getModifiers()));
        assertEquals("onImportResult must return void",
                void.class, method.getReturnType());
    }
}
