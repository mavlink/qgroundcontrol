/**
 * USB serial ports for the C++ side of QGroundControl, which reaches them through JNI
 * (src/Android/qtandroidserialport/AndroidSerial.cc).
 *
 * <h2>Classes</h2>
 * <ul>
 *   <li>{@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager} is what C++ calls. It holds no logic of its
 *       own: it finds the port a call is for and turns results and exceptions into the values C++ expects.
 *   <li>{@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialDevices} tracks the attached devices that have a
 *       serial driver, whether QGC may open them, and the open ports. It lists a device that QGC may not open
 *       too, and asks the user for the permission when a port of it is opened, if the device's
 *       {@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialPermissionPrompt} calls for that.
 *       {@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialPortName} is how a port is named.
 *   <li>{@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialPort} is one open port.
 *       {@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialReader} is the port's input, with its read thread,
 *       and {@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialWriter} its output, with its write thread.
 *   <li>{@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialAndroidHost} is the only class that touches Android's
 *       USB host API and its broadcasts.
 * </ul>
 * The drivers for the USB serial chips come from the library usb-serial-for-android.
 *
 * <h2>Threads</h2>
 * <ul>
 *   <li>C++ calls a port's methods on the thread of the C++ port that opened it, and asks for the port list on
 *       any thread.
 *   <li>Each open port reads on a thread of its own and, once it has been written to, writes on another. Both
 *       call back into C++ ({@link org.mavlink.qgroundcontrol.serial.QGCUsbSerialCallbacks}). While C++ has no
 *       room for input, the read thread asks the device for no more than the read transfers already queued
 *       with it. Closing the port waits for neither thread: every open gets a driver port of its own from the
 *       library, so a thread that ends late finds its driver port closed, whatever is open on the device by then.
 *   <li>USB broadcasts (attached, detached, permission answered) are handled on one more thread, owned by the
 *       host class. A detach closes the device's ports there and tells C++.
 *   <li>The activity starts and stops all of this on the UI thread, and reports there whether QGC's window has
 *       the focus. Starting waits for no device. Stopping, as the app ends, closes the ports that are still
 *       open, and a driver may wait for its device as it closes one.
 * </ul>
 *
 * <h2>Locks</h2>
 * <ul>
 *   <li>The devices' lock guards the tracked devices and the table of open ports. It is held for lookups and
 *       updates only: never during a USB transfer, a call into Android's USB service, a call on a port, or a
 *       call into C++. A call on an open port finds the port by its handle without the lock.
 *   <li>A port's lock serializes opening, closing and the starting of its threads. Opening holds it across USB
 *       transfers; reads and writes run without it, so that a close never waits for one.
 *   <li>A reader's lock and a writer's lock each guard which thread is its I/O thread, and the read thread
 *       waits on the reader's while C++ has no room for input. Each is taken under the port's lock or alone,
 *       and nothing is called under it but the start of that thread.
 *   <li>The devices' sync lock makes the syncs with the attached devices, which the start of the app and each
 *       USB event run, take effect one at a time. It is taken before either of the other two, and released
 *       before a sync closes the ports of a device that is gone and tells C++.
 * </ul>
 */
package org.mavlink.qgroundcontrol.serial;
