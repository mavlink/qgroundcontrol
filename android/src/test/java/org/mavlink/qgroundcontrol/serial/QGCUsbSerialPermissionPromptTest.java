package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;

import org.junit.Test;
import org.mavlink.qgroundcontrol.serial.QGCUsbSerialPermissionPrompt.Refusal;

public class QGCUsbSerialPermissionPromptTest {
    private static final long ATTACHED_AT_MS = 1000;
    // The earliest time at which Android's own prompt for the device counts as up.
    private static final long PROMPT_UP_AT_MS = ATTACHED_AT_MS + QGCUsbSerialPermissionPrompt.ATTACH_GRACE_MS;
    private static final String ASKED = "The user has been asked to allow it";
    private static final String NOT_ANSWERED = "The user has not answered the request yet";

    private final QGCUsbSerialPermissionPrompt _prompt = new QGCUsbSerialPermissionPrompt(ATTACHED_AT_MS);

    @Test
    public void refuseOpen_asksTheUserOnce() {
        assertEquals(new Refusal(true, true, ASKED), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
        assertEquals(new Refusal(false, false, NOT_ANSWERED), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_afterADenial_doesNotAskAgain() {
        _prompt.refuseOpen(PROMPT_UP_AT_MS, true);
        _prompt.denied();

        assertEquals(new Refusal(false, false,
                             "The user denied it; replug the device or connect to it by hand to be asked again"),
                     _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_rightAfterTheAttach_leavesTheAskingToAndroid() {
        final String justAttached = "The device was just attached, and Android asks the user first";
        assertEquals(new Refusal(false, true, justAttached), _prompt.refuseOpen(ATTACHED_AT_MS, true));
        assertEquals(new Refusal(false, false, justAttached), _prompt.refuseOpen(PROMPT_UP_AT_MS - 1, true));

        assertEquals(new Refusal(true, false, ASKED), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_whileTheUserCannotBeAsked_asksOnceTheyCan() {
        assertEquals(new Refusal(false, true, "The user is not asked while another window has the focus"),
                     _prompt.refuseOpen(PROMPT_UP_AT_MS, false));

        assertEquals(new Refusal(true, false, ASKED), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }

    @Test
    public void refuseOpen_whoseRequestAndroidClosedWithoutAnAnswer_asksAgain() {
        final long dialogGoneAtMs = PROMPT_UP_AT_MS + QGCUsbSerialPermissionPrompt.REQUEST_GRACE_MS;
        _prompt.refuseOpen(PROMPT_UP_AT_MS, true);

        // The dialog of the request takes a moment to cover QGC's window, and then keeps the user from being asked.
        assertEquals(new Refusal(false, false, NOT_ANSWERED), _prompt.refuseOpen(dialogGoneAtMs - 1, true));
        assertEquals(new Refusal(false, false, NOT_ANSWERED), _prompt.refuseOpen(dialogGoneAtMs, false));

        // Android reports no answer for a dialog that it closes as another prompt of its own is accepted on top.
        assertEquals(new Refusal(true, false, ASKED), _prompt.refuseOpen(dialogGoneAtMs, true));
        assertEquals(new Refusal(false, false, NOT_ANSWERED), _prompt.refuseOpen(dialogGoneAtMs, true));
    }

    @Test
    public void asked_countsAsTheRequestOfAnOpenAlsoAfterADenial() {
        _prompt.asked(PROMPT_UP_AT_MS);
        assertEquals(new Refusal(false, true, NOT_ANSWERED), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));

        _prompt.denied();
        _prompt.asked(PROMPT_UP_AT_MS);
        assertEquals(new Refusal(false, false, NOT_ANSWERED), _prompt.refuseOpen(PROMPT_UP_AT_MS, true));
    }
}
