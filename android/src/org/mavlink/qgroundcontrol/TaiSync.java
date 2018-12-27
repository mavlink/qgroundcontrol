package org.mavlink.qgroundcontrol;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import android.os.ParcelFileDescriptor;
import android.util.Log;

public class TaiSync
{
    private static final int PROTOCOL_VERSION = 0x1;
    private static final int PROTOCOL_CHANNEL = 0x02;
    private static final int PROTOCOL_DATA = 0x03;

    private static final int TELEM_PORT = 14550;
    private static final int VIDEO_PORT = 5000;

    private boolean running = false;
    private DatagramSocket socket = null;
    private ParcelFileDescriptor mParcelFileDescriptor;
    private FileInputStream mFileInputStream;
    private FileOutputStream mFileOutputStream;
    private ExecutorService mThreadPool;
    private byte[] mBytes = new byte[32 * 1024];
    private byte vMaj = 0;

    public TaiSync()
    {
        mThreadPool = Executors.newFixedThreadPool(2);
    }

    public boolean isRunning()
    {
        synchronized (this)
        {
            return running;
        }
    }

    public void open(ParcelFileDescriptor parcelFileDescriptor) throws IOException
    {
//        Log.i("QGC_TaiSync", "Open");

        synchronized (this)
        {
            if (running) {
                return;
            }
            running = true;
        }

        mParcelFileDescriptor = parcelFileDescriptor;
        if (mParcelFileDescriptor == null) {
            throw new IOException("parcelFileDescriptor is null");
        }

        FileDescriptor fileDescriptor = mParcelFileDescriptor.getFileDescriptor();
        mFileInputStream = new FileInputStream(fileDescriptor);
        mFileOutputStream = new FileOutputStream(fileDescriptor);

        socket = new DatagramSocket();
        final InetAddress address = InetAddress.getByName("localhost");

        // Request connection packet
        byte[] conn = { 0x00, 0x00, 0x00, 0x00,   // uint32 - protocol
                        0x00, 0x00, 0x00, 0x00,   // uint32 - dport
                        0x00, 0x00, 0x00, 0x1C,   // uint32 - length
                        0x00, 0x00, 0x00, 0x00,   // uint32 - magic
                        0x00, 0x00, 0x00, 0x00,   // uint32 - version major
                        0x00, 0x00, 0x00, 0x00,   // uint32 - version minor
                        0x00, 0x00, 0x00, 0x00 }; // uint32 - padding
        mFileOutputStream.write(conn);

        mThreadPool.execute(new Runnable() {
            @Override
            public void run()
            {
                int bytesRead = 0;

                try {
                    while (bytesRead >= 0) {
                        synchronized (this)
                        {
                            if (!running) {
                                break;
                            }
                        }

                        bytesRead = mFileInputStream.read(mBytes);

                        if (bytesRead > 0)
                        {
                            if (mBytes[3] == PROTOCOL_VERSION)
                            {
                                vMaj = mBytes[19];
                                Log.i("QGC_TaiSync", "Got protocol version message vMaj = " + mBytes[19]);
                                byte[] data = { 0x00, 0x00, 0x00, 0x01,   // uint32 - protocol
                                                0x00, 0x00, 0x00, 0x00,   // uint32 - dport
                                                0x00, 0x00, 0x00, 0x1C,   // uint32 - length
                                                0x00, 0x00, 0x00, 0x00,   // uint32 - magic
                                                0x00, 0x00, 0x00, vMaj,   // uint32 - version major
                                                0x00, 0x00, 0x00, 0x00,   // uint32 - version minor
                                                0x00, 0x00, 0x00, 0x00 }; // uint32 - padding
                                mFileOutputStream.write(data);
                            }
                            else if (mBytes[3] == PROTOCOL_CHANNEL) {
                                int dPort = ((mBytes[4] & 0xff)<< 24) | ((mBytes[5]&0xff) << 16) | ((mBytes[6]&0xff) << 8) | (mBytes[7] &0xff);
                                int dLength = ((mBytes[8] & 0xff)<< 24) | ((mBytes[9]&0xff) << 16) | ((mBytes[10]&0xff) << 8) | (mBytes[11] &0xff);
                                Log.i("QGC_TaiSync", "Read 2 port = " + dPort + " length = " + dLength);
                                byte[] data = { 0x00, 0x00, 0x00, 0x02,       // uint32 - protocol
                                                0x00, 0x00, mBytes[6], mBytes[7], // uint32 - dport
                                                0x00, 0x00, 0x00, 0x1C,       // uint32 - length
                                                0x00, 0x00, 0x00, 0x00,       // uint32 - magic
                                                0x00, 0x00, 0x00, vMaj,       // uint32 - version major
                                                0x00, 0x00, 0x00, 0x00,       // uint32 - version minor
                                                0x00, 0x00, 0x00, 0x00 };     // uint32 - padding
                                 mFileOutputStream.write(data);
                            }
                            else if (mBytes[3] == PROTOCOL_DATA) {
                                int dPort = ((mBytes[4] & 0xff)<< 24) | ((mBytes[5]&0xff) << 16) | ((mBytes[6]&0xff) << 8) | (mBytes[7] &0xff);
                                int dLength = ((mBytes[8] & 0xff)<< 24) | ((mBytes[9]&0xff) << 16) | ((mBytes[10]&0xff) << 8) | (mBytes[11] &0xff);

                                int payloadOffset = 28;
                                int payloadLength = bytesRead - payloadOffset;

                                if (dPort == 8000) { // Video
                                    byte[] sBytes = new byte[payloadLength];
                                    System.arraycopy(mBytes, payloadOffset, sBytes, 0, payloadLength);
                                    DatagramPacket packet = new DatagramPacket(sBytes, sBytes.length, address, VIDEO_PORT);
                                    socket.send(packet);

                                } else if (dPort == 8200) { // Command
/*
                                    StringBuilder sb = new StringBuilder();
                                    for (int i = 0; i < dLength; i++) {
                                        sb.append(String.format("%02X ", mBytes[i]));
                                    }
                                    Log.i("QGC_TaiSync", "Read 3 port = " + dPort + " length = " + dLength + " - " + sb.toString());
*/
                                } else if (dPort == 8400) { // Telemetry
                                    byte[] sBytes = new byte[payloadLength];
                                    System.arraycopy(mBytes, payloadOffset, sBytes, 0, payloadLength);
                                    DatagramPacket packet = new DatagramPacket(sBytes, sBytes.length, address, TELEM_PORT);
                                    socket.send(packet);
                                }
                            }
                        }
                    }
                } catch (IOException e) {
                    Log.e("QGC_TaiSync", "Exception: " + e);
                    e.printStackTrace();
                } finally {
                    close();
                }
            }
        });

        mThreadPool.execute(new Runnable() {
            @Override
            public void run()
            {
                byte[] inbuf = new byte[256];

                try {
                    while (true) {
                        synchronized (this)
                        {
                            if (!running) {
                                break;
                            }
                        }

                        DatagramPacket packet = new DatagramPacket(inbuf, inbuf.length);
                        socket.receive(packet);

                        int dataPort = 8400;
                        byte portMSB = (byte)((dataPort >> 8) & 0xFF);
                        byte portLSB = (byte)(dataPort & 0xFF);

                        byte[] lA = new byte[4];
                        int len = 28 + packet.getLength();
                        byte[] buffer = new byte[len];

                        for (int i = 3; i >= 0; i--) {
                            lA[i] = (byte)(len & 0xFF);
                            len >>= 8;
                        }

                        byte[] header = { 0x00, 0x00, 0x00, PROTOCOL_DATA, // uint32 - protocol
                                        0x00, 0x00, portMSB, portLSB,      // uint32 - dport
                                        lA[0], lA[1], lA[2], lA[3],        // uint32 - length
                                        0x00, 0x00, 0x00, 0x00,            // uint32 - magic
                                        0x00, 0x00, 0x00, vMaj,            // uint32 - version major
                                        0x00, 0x00, 0x00, 0x00,            // uint32 - version minor
                                        0x00, 0x00, 0x00, 0x00 };          // uint32 - padding

                        System.arraycopy(header, 0, buffer, 0, header.length);
                        System.arraycopy(packet.getData(), 0, buffer, header.length, packet.getLength());
                        mFileOutputStream.write(buffer);
                    }
                } catch (IOException e) {
                    Log.e("QGC_TaiSync", "Exception: " + e);
                    e.printStackTrace();
                } finally {
                    close();
                }
            }
        });
    }

    public void close()
    {
//        Log.i("QGC_TaiSync", "Close");
        synchronized (this)
        {
            running = false;
        }
        try {
            if (socket != null) {
                socket.close();
                socket = null;
            }
            if (mParcelFileDescriptor != null) {
                mParcelFileDescriptor.close();
                mParcelFileDescriptor = null;
            }
        } catch (Exception e) {
        }
        socket = null;
    }
}
