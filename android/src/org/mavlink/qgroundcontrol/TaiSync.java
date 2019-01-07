package org.mavlink.qgroundcontrol;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Socket;
import java.net.InetAddress;

import android.os.ParcelFileDescriptor;
import android.util.Log;

public class TaiSync
{
    private static final int HEADER_SIZE = 0x1C;

    private static final byte PROTOCOL_REQUEST_CONNECTION = 0x00;
    private static final byte PROTOCOL_VERSION = 0x01;
    private static final byte PROTOCOL_CHANNEL = 0x02;
    private static final byte PROTOCOL_DATA = 0x03;

    private static final int TELEM_PORT = 14550;
    private static final int VIDEO_PORT = 5000;
    private static final int TAISYNC_VIDEO_PORT = 8000;
    private static final int TAISYNC_SETTINGS_PORT = 8200;
    private static final int TAISYNC_TELEMETRY_PORT = 8400;

    private boolean running = false;
    private DatagramSocket udpSocket = null;
    private Socket tcpSocket = null;
    private InputStream tcpInStream = null;
    private OutputStream tcpOutStream = null;
    private ParcelFileDescriptor mParcelFileDescriptor;
    private FileInputStream mFileInputStream;
    private FileOutputStream mFileOutputStream;
    private ExecutorService mThreadPool;
    private byte[] mBytes = new byte[32 * 1024];
    private byte vMaj = 0;

    public TaiSync()
    {
        mThreadPool = Executors.newFixedThreadPool(3);
    }

    public boolean isRunning()
    {
        synchronized (this) {
            return running;
        }
    }

    public void open(ParcelFileDescriptor parcelFileDescriptor) throws IOException
    {
//        Log.i("QGC_TaiSync", "Open");

        synchronized (this) {
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

        udpSocket = new DatagramSocket();
        final InetAddress address = InetAddress.getByName("localhost");
        tcpSocket = new Socket(address, TAISYNC_SETTINGS_PORT);
        tcpInStream = tcpSocket.getInputStream();
        tcpOutStream = tcpSocket.getOutputStream();

        // Request connection packet
        sendTaiSyncMessage(PROTOCOL_REQUEST_CONNECTION, 0, null, 0);

        // Read multiplexed data stream coming from TaiSync accessory
        mThreadPool.execute(new Runnable() {
            @Override
            public void run()
            {
                int bytesRead = 0;

                try {
                    while (bytesRead >= 0) {
                        synchronized (this) {
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
                                sendTaiSyncMessage(PROTOCOL_VERSION, 0, null, 0);
                            }
                            else if (mBytes[3] == PROTOCOL_CHANNEL) {
                                int dPort = ((mBytes[4] & 0xff)<< 24) | ((mBytes[5]&0xff) << 16) | ((mBytes[6]&0xff) << 8) | (mBytes[7] &0xff);
                                int dLength = ((mBytes[8] & 0xff)<< 24) | ((mBytes[9]&0xff) << 16) | ((mBytes[10]&0xff) << 8) | (mBytes[11] &0xff);
                                Log.i("QGC_TaiSync", "Read 2 port = " + dPort + " length = " + dLength);
                                sendTaiSyncMessage(PROTOCOL_CHANNEL, dPort, null, 0);
                            }
                            else if (mBytes[3] == PROTOCOL_DATA) {
                                int dPort = ((mBytes[4] & 0xff)<< 24) | ((mBytes[5]&0xff) << 16) | ((mBytes[6]&0xff) << 8) | (mBytes[7] &0xff);
                                int dLength = ((mBytes[8] & 0xff)<< 24) | ((mBytes[9]&0xff) << 16) | ((mBytes[10]&0xff) << 8) | (mBytes[11] &0xff);

                                int payloadOffset = HEADER_SIZE;
                                int payloadLength = bytesRead - payloadOffset;

                                byte[] sBytes = new byte[payloadLength];
                                System.arraycopy(mBytes, payloadOffset, sBytes, 0, payloadLength);

                                if (dPort == TAISYNC_VIDEO_PORT) {
                                    DatagramPacket packet = new DatagramPacket(sBytes, sBytes.length, address, VIDEO_PORT);
                                    udpSocket.send(packet);
                                } else if (dPort == TAISYNC_SETTINGS_PORT) {
                                    tcpOutStream.write(sBytes);
                                } else if (dPort == TAISYNC_TELEMETRY_PORT) {
                                    DatagramPacket packet = new DatagramPacket(sBytes, sBytes.length, address, TELEM_PORT);
                                    udpSocket.send(packet);
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

        // Read command & control packets to be sent to telemetry port
        mThreadPool.execute(new Runnable() {
            @Override
            public void run()
            {
                byte[] inbuf = new byte[256];

                try {
                    while (true) {
                        synchronized (this) {
                            if (!running) {
                                break;
                            }
                        }

                        DatagramPacket packet = new DatagramPacket(inbuf, inbuf.length);
                        udpSocket.receive(packet);
                        sendTaiSyncMessage(PROTOCOL_DATA, TAISYNC_TELEMETRY_PORT, packet.getData(), packet.getLength());
                    }
                } catch (IOException e) {
                    Log.e("QGC_TaiSync", "Exception: " + e);
                    e.printStackTrace();
                } finally {
                    close();
                }
            }
        });

        // Read incoming requests for settings socket
        mThreadPool.execute(new Runnable() {
            @Override
            public void run()
            {
                byte[] inbuf = new byte[1024];

                try {
                    while (true) {
                        synchronized (this) {
                            if (!running) {
                                break;
                            }
                        }

                        int bytesRead = tcpInStream.read(inbuf, 0, inbuf.length);
                        if (bytesRead > 0) {
                            sendTaiSyncMessage(PROTOCOL_DATA, TAISYNC_SETTINGS_PORT, inbuf, bytesRead);
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
    }

    private void sendTaiSyncMessage(byte protocol, int dataPort, byte[] data, int dataLen) throws IOException
    {
        byte portMSB = (byte)((dataPort >> 8) & 0xFF);
        byte portLSB = (byte)(dataPort & 0xFF);

        byte[] lA = new byte[4];
        int len = HEADER_SIZE + dataLen;
        byte[] buffer = new byte[len];

        for (int i = 3; i >= 0; i--) {
            lA[i] = (byte)(len & 0xFF);
            len >>= 8;
        }

        byte[] header = { 0x00, 0x00, 0x00, protocol,   // uint32 - protocol
                          0x00, 0x00, portMSB, portLSB, // uint32 - dport
                          lA[0], lA[1], lA[2], lA[3],   // uint32 - length
                          0x00, 0x00, 0x00, 0x00,       // uint32 - magic
                          0x00, 0x00, 0x00, vMaj,       // uint32 - version major
                          0x00, 0x00, 0x00, 0x00,       // uint32 - version minor
                          0x00, 0x00, 0x00, 0x00 };     // uint32 - padding

        System.arraycopy(header, 0, buffer, 0, header.length);
        if (data != null && dataLen > 0) {
            System.arraycopy(data, 0, buffer, header.length, dataLen);
        }

        synchronized (this) {
            mFileOutputStream.write(buffer);
        }
    }

    public void close()
    {
//        Log.i("QGC_TaiSync", "Close");
        synchronized (this) {
            running = false;
        }
        try {
            if (udpSocket != null) {
                udpSocket.close();
            }
        } catch (Exception e) {
        }
        try {
            if (tcpSocket != null) {
                tcpSocket.close();
            }
        } catch (Exception e) {
        }
        try {
            if (mParcelFileDescriptor != null) {
                mParcelFileDescriptor.close();
            }
        } catch (Exception e) {
        }
        udpSocket = null;
        tcpSocket = null;
        tcpInStream = null;
        tcpOutStream = null;
        mParcelFileDescriptor = null;
    }
}
