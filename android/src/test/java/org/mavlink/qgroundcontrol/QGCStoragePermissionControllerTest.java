package org.mavlink.qgroundcontrol;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.pm.PackageManager;

import org.junit.Test;

public class QGCStoragePermissionControllerTest {

    @Test
    public void requiresRuntimeStoragePermission_isSdkGatedCorrectly() {
        assertTrue(QGCStoragePermissionController.requiresRuntimeStoragePermission(29));
        assertFalse(QGCStoragePermissionController.requiresRuntimeStoragePermission(30));
    }

    @Test
    public void areAllPermissionsGranted_handlesEmptyAndDeniedResults() {
        assertFalse(QGCStoragePermissionController.areAllPermissionsGranted(new int[]{}));
        assertFalse(QGCStoragePermissionController.areAllPermissionsGranted(
            new int[]{PackageManager.PERMISSION_GRANTED, PackageManager.PERMISSION_DENIED}
        ));
    }

    @Test
    public void areAllPermissionsGranted_returnsTrueWhenEveryResultGranted() {
        assertTrue(QGCStoragePermissionController.areAllPermissionsGranted(
            new int[]{PackageManager.PERMISSION_GRANTED, PackageManager.PERMISSION_GRANTED}
        ));
    }

    @Test
    public void onRequestPermissionsResult_ignoresOtherRequestCodes() {
        final QGCStoragePermissionController controller = new QGCStoragePermissionController(null);
        controller.setStoragePermissionRequestInFlightForTesting(true);

        assertNull(controller.onRequestPermissionsResult(999, new int[]{PackageManager.PERMISSION_GRANTED}));
        assertTrue(controller.isStoragePermissionRequestInFlightForTesting());
    }

    @Test
    public void onRequestPermissionsResult_matchingCodeResetsInFlightAndReturnsGrantDecision() {
        final QGCStoragePermissionController controller = new QGCStoragePermissionController(null);

        controller.setStoragePermissionRequestInFlightForTesting(true);
        assertTrue(controller.onRequestPermissionsResult(
            QGCStoragePermissionController.STORAGE_PERMISSION_REQUEST_CODE,
            new int[]{PackageManager.PERMISSION_GRANTED}
        ));
        assertFalse(controller.isStoragePermissionRequestInFlightForTesting());

        controller.setStoragePermissionRequestInFlightForTesting(true);
        assertFalse(controller.onRequestPermissionsResult(
            QGCStoragePermissionController.STORAGE_PERMISSION_REQUEST_CODE,
            new int[]{PackageManager.PERMISSION_DENIED}
        ));
        assertFalse(controller.isStoragePermissionRequestInFlightForTesting());
    }
}
