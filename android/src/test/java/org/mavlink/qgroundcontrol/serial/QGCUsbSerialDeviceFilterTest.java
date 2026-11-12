package org.mavlink.qgroundcontrol.serial;

import static org.junit.Assert.assertTrue;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import javax.xml.parsers.DocumentBuilderFactory;

import org.junit.Test;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

/**
 * The manifest's USB device filter against the devices that the serial prober finds. Android asks the user
 * about a newly attached device only if the filter matches it, and {@link QGCUsbSerialDevices} leaves the
 * asking to Android, so a device that the prober finds and the filter misses could not be used.
 */
public class QGCUsbSerialDeviceFilterTest {
    private static final int ANY = -1;

    /** One usb-device entry of the filter. An attribute that it leaves out matches {@link #ANY} value. */
    private record FilterEntry(int vendorId, int productId, int deviceClass, int subclass, int protocol) {
        boolean matchesProduct(final int vendor, final int product) {
            // Nothing is known of the product's interfaces, so an entry that asks for some does not count.
            return (vendorId == ANY || vendorId == vendor) && (productId == ANY || productId == product)
                    && deviceClass == ANY && subclass == ANY && protocol == ANY;
        }

        boolean matchesInterface(final int interfaceClass, final int interfaceSubclass) {
            return vendorId == ANY && productId == ANY && (deviceClass == ANY || deviceClass == interfaceClass)
                    && (subclass == ANY || subclass == interfaceSubclass) && protocol == ANY;
        }
    }

    private static int attribute(final Element entry, final String name) {
        return entry.hasAttribute(name) ? Integer.parseInt(entry.getAttribute(name)) : ANY;
    }

    private static List<FilterEntry> filterEntries() throws Exception {
        // Unit tests run in the directory of the Gradle project.
        final NodeList nodes = DocumentBuilderFactory.newInstance().newDocumentBuilder()
                .parse(new File("res/xml/device_filter.xml")).getElementsByTagName("usb-device");
        final List<FilterEntry> entries = new ArrayList<>();
        for (int i = 0; i < nodes.getLength(); i++) {
            final Element entry = (Element) nodes.item(i);
            entries.add(new FilterEntry(attribute(entry, "vendor-id"), attribute(entry, "product-id"),
                    attribute(entry, "class"), attribute(entry, "subclass"), attribute(entry, "protocol")));
        }
        return entries;
    }

    /**
     * The products that the library's drivers claim by ID, as {vendor ID, product ID}, asked of every driver
     * class in the library, so that a driver which a new version adds is included.
     */
    private static List<int[]> libraryProducts() throws Exception {
        final List<int[]> products = new ArrayList<>();
        final String driverDirectory = UsbSerialDriver.class.getPackage().getName().replace('.', '/') + "/";
        final File library =
                new File(UsbSerialDriver.class.getProtectionDomain().getCodeSource().getLocation().toURI());
        try (JarFile jar = new JarFile(library)) {
            for (final Enumeration<JarEntry> entries = jar.entries(); entries.hasMoreElements();) {
                final String name = entries.nextElement().getName();
                if (!name.startsWith(driverDirectory) || !name.endsWith(".class")) {
                    continue;
                }
                final String className = name.substring(0, name.length() - ".class".length()).replace('/', '.');
                final Method supportedDevices;
                try {
                    supportedDevices = Class.forName(className).getMethod("getSupportedDevices");
                } catch (final NoSuchMethodException e) {
                    continue;
                }
                @SuppressWarnings("unchecked")
                final Map<Integer, int[]> devices = (Map<Integer, int[]>) supportedDevices.invoke(null);
                devices.forEach((vendorId, productIds) -> {
                    for (final int productId : productIds) {
                        products.add(new int[] {vendorId, productId});
                    }
                });
            }
        }
        return products;
    }

    @Test
    public void filter_listsEveryProductThatTheProberKnowsById() throws Exception {
        final List<int[]> products = libraryProducts();
        // The library's own FT232R shows that its drivers were found and asked.
        assertTrue(products.stream().anyMatch(product -> product[0] == 0x0403 && product[1] == 0x6001));
        products.addAll(List.of(QGCUsbSerialProber.ADDED_FTDI_PRODUCTS));

        final List<FilterEntry> entries = filterEntries();
        for (final int[] product : products) {
            assertTrue(String.format(Locale.ROOT, "device_filter.xml lacks <usb-device vendor-id=\"%d\" "
                                     + "product-id=\"%d\" /> for %04x:%04x", product[0], product[1], product[0],
                                     product[1]),
                       entries.stream().anyMatch(entry -> entry.matchesProduct(product[0], product[1])));
        }
    }

    @Test
    public void filter_matchesCdcAcmDevicesByTheirInterface() throws Exception {
        // The library claims any device that has a CDC-ACM control interface with a data interface, whatever
        // its IDs: communications class 2, ACM subclass 2.
        assertTrue(filterEntries().stream().anyMatch(entry -> entry.matchesInterface(2, 2)));
    }
}
