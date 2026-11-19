package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialResult.PERMISSION_AWAITS_ANDROID;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialResult.PERMISSION_AWAITS_FOCUS;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialResult.PERMISSION_DENIED;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialResult.PERMISSION_PENDING;
import static org.mavlink.qgroundcontrol.serial.QGCUsbSerialResult.PERMISSION_REQUESTED;

import org.junit.Test;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialPermissionPrompt.Refusal;

public class QGCUsbSerialPermissionPromptTest {
    private static final long ATTACHED_AT_MS = 1000;
    // The earliest time at which Android's own prompt for the device counts as up.
    private static final long PROMPT_UP_AT_MS = ATTACHED_AT_MS + QGCUsbSerialPermissionPrompt.ATTACH_GRACE_MS;

    private final QGCUsbSerialPermissionPrompt _prompt = new QGCUsbSerialPermissionPrompt(ATTACHED_AT_MS);

    @Test
    public void refuseOpen_asksTheUserOnce() {
        assertEquals(new Refusal(PERMISSION_REQUESTED, true), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
        assertEquals(new Refusal(PERMISSION_PENDING, false), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_afterADenial_doesNotAskAgain() {
        _prompt.refuseOpen(PROMPT_UP_AT_MS, true);
        _prompt.denied();

        assertEquals(new Refusal(PERMISSION_DENIED, false), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_rightAfterTheAttach_leavesTheAskingToAndroid() {
        assertEquals(new Refusal(PERMISSION_AWAITS_ANDROID, true), _prompt.refuseOpen(ATTACHED_AT_MS, true));
        assertEquals(new Refusal(PERMISSION_AWAITS_ANDROID, false), _prompt.refuseOpen(PROMPT_UP_AT_MS - 1, true));

        assertEquals(new Refusal(PERMISSION_REQUESTED, false), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_whileTheUserCannotBeAsked_asksOnceTheyCan() {
        assertEquals(new Refusal(PERMISSION_AWAITS_FOCUS, true), _prompt.refuseOpen(PROMPT_UP_AT_MS, false));

        assertEquals(new Refusal(PERMISSION_REQUESTED, false), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_whoseRequestAndroidClosedWithoutAnAnswer_asksAgain() {
        final long dialogGoneAtMs = PROMPT_UP_AT_MS + QGCUsbSerialPermissionPrompt.REQUEST_GRACE_MS;
        _prompt.refuseOpen(PROMPT_UP_AT_MS, true);

        // The dialog of the request takes a moment to cover QGC's window, and then keeps the user from being asked.
        assertEquals(new Refusal(PERMISSION_PENDING, false), _prompt.refuseOpen(dialogGoneAtMs - 1, true));
        assertEquals(new Refusal(PERMISSION_PENDING, false), _prompt.refuseOpen(dialogGoneAtMs, false));

        // Android reports no answer for a dialog that it closes as another prompt of its own is accepted on top.
        assertEquals(new Refusal(PERMISSION_REQUESTED, false), _prompt.refuseOpen(dialogGoneAtMs, true));
        assertEquals(new Refusal(PERMISSION_PENDING, false), _prompt.refuseOpen(dialogGoneAtMs, true));
    }

    @Test
    public void asked_countsAsTheRequestOfAnOpenAlsoAfterADenial() {
        _prompt.asked(PROMPT_UP_AT_MS);
        assertEquals(new Refusal(PERMISSION_PENDING, true), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));

        _prompt.denied();
        _prompt.asked(PROMPT_UP_AT_MS);
        assertEquals(new Refusal(PERMISSION_PENDING, false), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }
}
