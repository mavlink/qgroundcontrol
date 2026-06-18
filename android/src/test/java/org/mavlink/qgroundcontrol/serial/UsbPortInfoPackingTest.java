package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Application;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Set;
import java.util.TreeSet;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class)
public class UsbPortInfoPackingTest {

    private static JSONObject parse(final byte[] packed) throws JSONException {
        return new JSONObject(new String(packed, StandardCharsets.UTF_8));
    }

    @Test
    public void emptyArray_packsEmptyPortsList() throws JSONException {
        final JSONObject root = parse(QGCUsbSerialManager.packPortsInfo(new UsbPortInfo[0]));
        assertEquals(0, root.getJSONArray("ports").length());
    }

    @Test
    public void singlePort_roundTripsAllFieldsInRecordOrder() throws JSONException {
        final UsbPortInfo info = new UsbPortInfo("/dev/ttyACM0", "Pixhawk", "ArduPilot", "SN123",
                0x0011, 0x26ac, new int[] { 57600, 115200, 921600 });
        final JSONObject port = parse(QGCUsbSerialManager.packPortsInfo(new UsbPortInfo[] { info }))
                .getJSONArray("ports").getJSONObject(0);

        assertEquals("/dev/ttyACM0", port.getString("deviceName"));
        assertEquals("Pixhawk", port.getString("productName"));
        assertEquals("ArduPilot", port.getString("manufacturerName"));
        assertEquals("SN123", port.getString("serialNumber"));
        assertEquals(0x0011, port.getInt("productId"));
        assertEquals(0x26ac, port.getInt("vendorId"));
        final JSONArray bauds = port.getJSONArray("baudRates");
        assertEquals(3, bauds.length());
        assertEquals(57600, bauds.getInt(0));
        assertEquals(115200, bauds.getInt(1));
        assertEquals(921600, bauds.getInt(2));
    }

    @Test
    public void nullString_omitsKeyDistinctFromEmpty() throws JSONException {
        final UsbPortInfo info = new UsbPortInfo("dev", null, "", null, 1, 2, new int[0]);
        final JSONObject port = parse(QGCUsbSerialManager.packPortsInfo(new UsbPortInfo[] { info }))
                .getJSONArray("ports").getJSONObject(0);

        assertEquals("dev", port.getString("deviceName"));
        assertFalse("null string omitted", port.has("productName"));
        assertTrue("empty string present", port.has("manufacturerName"));
        assertEquals("", port.getString("manufacturerName"));
        assertFalse(port.has("serialNumber"));
        assertEquals(0, port.getJSONArray("baudRates").length());
    }

    @Test
    public void nullBaudArray_packsEmptyBaudList() throws JSONException {
        final UsbPortInfo info = new UsbPortInfo("dev", "p", "m", "s", 1, 2, null);
        final JSONObject port = parse(QGCUsbSerialManager.packPortsInfo(new UsbPortInfo[] { info }))
                .getJSONArray("ports").getJSONObject(0);
        assertEquals(0, port.getJSONArray("baudRates").length());
    }

    @Test
    public void utf8MultiByte_survivesRoundTrip() throws JSONException {
        final UsbPortInfo info = new UsbPortInfo("café", "", "", "", 0, 0, new int[0]);
        final JSONObject port = parse(QGCUsbSerialManager.packPortsInfo(new UsbPortInfo[] { info }))
                .getJSONArray("ports").getJSONObject(0);
        assertEquals("café", port.getString("deviceName"));
    }

    @Test
    public void multiplePorts_packCountAndOrderPreserved() throws JSONException {
        final UsbPortInfo a = new UsbPortInfo("a", "", "", "", 1, 1, new int[] { 9600 });
        final UsbPortInfo b = new UsbPortInfo("b", "", "", "", 2, 2, new int[] { 19200, 38400 });
        final JSONArray ports = parse(QGCUsbSerialManager.packPortsInfo(new UsbPortInfo[] { a, b }))
                .getJSONArray("ports");

        assertEquals(2, ports.length());
        assertEquals("a", ports.getJSONObject(0).getString("deviceName"));
        assertEquals(9600, ports.getJSONObject(0).getJSONArray("baudRates").getInt(0));
        assertEquals("b", ports.getJSONObject(1).getString("deviceName"));
        assertEquals(38400, ports.getJSONObject(1).getJSONArray("baudRates").getInt(1));
    }

    // Packs against the same on-disk fixture the C++ SerialPortInfoCodecTest decodes, so a JSON key rename
    // on either side drifts from the shared golden literal and fails one suite.
    @Test
    public void goldenFixture_packerMatchesSharedContract() throws Exception {
        final JSONObject goldenPort = loadGolden().getJSONArray("ports").getJSONObject(0);
        final UsbPortInfo info = new UsbPortInfo("/dev/ttyACM0", "Pixhawk", "ArduPilot", "SN123",
                0x0011, 0x26ac, new int[] { 57600, 115200, 921600 });
        final JSONObject packed = parse(QGCUsbSerialManager.packPortsInfo(new UsbPortInfo[] { info }))
                .getJSONArray("ports").getJSONObject(0);

        assertEquals(keys(goldenPort), keys(packed));
        for (final String key : keys(goldenPort)) {
            assertEquals("value for key " + key, goldenPort.get(key).toString(), packed.get(key).toString());
        }
    }

    private static JSONObject loadGolden() throws Exception {
        try (InputStream in = UsbPortInfoPackingTest.class.getResourceAsStream("/PortInfoGolden.json")) {
            assertNotNull("golden fixture missing — check test resources.srcDirs in android/build.gradle", in);
            return new JSONObject(new String(in.readAllBytes(), StandardCharsets.UTF_8));
        }
    }

    private static Set<String> keys(final JSONObject obj) {
        final Set<String> result = new TreeSet<>();
        for (final java.util.Iterator<String> it = obj.keys(); it.hasNext(); ) {
            result.add(it.next());
        }
        return result;
    }
}
